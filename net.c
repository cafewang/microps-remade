#include <string.h>

#include "platform.h"

#include "util.h"
#include "net.h"
#include "ip.h"

typedef void (*protocol_handler)(const uint8_t *data, size_t len, struct net_device *dev);

struct net_protocol {
    struct net_protocol *next;
    uint16_t type;
    struct queue_head queue; /* input queue */
    protocol_handler handler;
};

struct net_protocol_queue_entry {
    struct net_device *dev;
    size_t len;
    uint8_t data[];
};

/* NOTE: if you want to add/delete the entries after net_run(), you need to protect these lists with a mutex. */
static struct net_device *devices;
static struct net_protocol *protocols;

struct net_device *
net_device_alloc(void)
{
    struct net_device *dev;

    dev = memory_alloc(sizeof(*dev));
    if (!dev) {
        errorf("memory_alloc() failure");
        return NULL;
    }
    return dev;
}

/* NOTE: must not be call after net_run() */
int
net_device_register(struct net_device *dev)
{
    static unsigned int index = 0;

    dev->index = index++;
    snprintf(dev->name, sizeof(dev->name), "net%d", dev->index);
    dev->next = devices;
    devices = dev;
    infof("registered, dev=%s, type=0x%04x", dev->name, dev->type);
    return 0;
}

static int
net_device_open(struct net_device *dev)
{
    if (NET_DEVICE_IS_UP(dev)) {
        errorf("already opened, dev=%s", dev->name);
        return -1;
    }
    if (dev->ops->open) {
        if (dev->ops->open(dev) == -1) {
            errorf("failure, dev=%s", dev->name);
            return -1;
        }
    }
    dev->flags |= NET_DEVICE_FLAG_UP;
    infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
    return 0;
}

static int
net_device_close(struct net_device *dev)
{
    if (!NET_DEVICE_IS_UP(dev)) {
        errorf("not opened, dev=%s", dev->name);
        return -1;
    }
    if (dev->ops->close) {
        if (dev->ops->close(dev) == -1) {
            errorf("failure, dev=%s", dev->name);
            return -1;
        }
    }
    dev->flags &= ~NET_DEVICE_FLAG_UP;
    infof("dev=%s, state=%s", dev->name, NET_DEVICE_STATE(dev));
    return 0;
}

int
net_device_output(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst)
{
    if (!NET_DEVICE_IS_UP(dev)) {
        errorf("not opened, dev=%s", dev->name);
        return -1;
    }
    if (len > dev->mtu) {
        errorf("too long, dev=%s, mtu=%u, len=%zu", dev->name, dev->mtu, len);
        return -1;
    }
    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    if (dev->ops->transmit(dev, type, data, len, dst) == -1) {
        errorf("device transmit failure, dev=%s, len=%zu", dev->name, len);
        return -1;
    }
    return 0;
}

static int protocol_registered(uint16_t type)
{
    struct net_protocol *p;

    for (p = protocols; p; p = p->next) {
        if (p->type == type) {
            return 1;
        }
    }
    return 0;
}

/* NOTE: must not be call after net_run() */
int
net_protocol_register(uint16_t type, void (*handler)(const uint8_t *data, size_t len, struct net_device *dev)) {
    struct net_protocol *p;
    int registered = protocol_registered(type);
    if (registered) {
        errorf("protocol already registered, type=0x%04x", type);
        return -1;
    }
    p = memory_alloc(sizeof(*p));
    if (!p) {
        errorf("memory_alloc() failure");
        return -1;
    }
    p->type = type;
    p->handler = handler;
    p->next = protocols;
    protocols = p;
    infof("protocol registered, type=0x%04x", type);
    return 0;
}

int
net_softirq_handler(void) {
    struct net_protocol *p;
    struct net_protocol_queue_entry *e;

    for (p = protocols; p; p = p->next) {
        while (1) {
            e = queue_pop(&p->queue);
            if (!e) {
                break;
            }
            debugf("queue poped(num left=%d), dev=%s, type=0x%04x, len=%zu", p->queue.num, e->dev->name, p->type, e->len);
            p->handler(e->data, e->len, e->dev);
            memory_free(e);
        }
    }
    return 0;
}


int net_input_handler(uint16_t type, const uint8_t *data, size_t len, struct net_device *dev) {
    struct net_protocol *p;
    struct net_protocol_queue_entry *e;

    for (p = protocols; p; p = p->next) {
        if (p->type == type) {
            e = memory_alloc(sizeof(*e) + len);
            if (!e) {
                errorf("memory_alloc() failure");
                return -1;
            }
            e->dev = dev;
            e->len = len;
            memcpy(e->data, data, len);
            queue_push(&p->queue, e);
            debugf("input packet, dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
            debugdump(data, len);
            intr_raise_irq(INTR_IRQ_SOFTIRQ);
            return 0;
        }
    }
    errorf("protocol not registered, type=0x%04x", type);
    return 0;
}

int
net_run(void)
{
    struct net_device *dev;

    if (intr_run() == -1) {
        errorf("intr_run() failure");
        return -1;
    }

    debugf("open all devices...");
    for (dev = devices; dev; dev = dev->next) {
        net_device_open(dev);
    }
    debugf("running...");
    return 0;
}

void
net_shutdown(void)
{
    struct net_device *dev;

    debugf("close all devices...");
    for (dev = devices; dev; dev = dev->next) {
        net_device_close(dev);
    }
    intr_shutdown();
    debugf("shutting down");
}

int
net_init(void)
{
    if (intr_init() == -1) {
        errorf("intr_init() failure");
        return -1;
    }
    if (ip_init() == -1) {
        errorf("ip_init() failure");
        return -1;
    }
    infof("initialized");
    return 0;
}


/* NOTE: must not be call after net_run() */
int net_device_add_iface(struct net_device *dev, struct net_iface *iface) {
    struct net_iface *entry;
    for (entry = dev->ifaces; entry; entry = entry->next) {
        if (entry->family == iface->family) {
            errorf("already added, dev=%s, family=%d", dev->name, iface->family);
            return -1;
        }
    }
    iface->dev = dev;
    iface->next = dev->ifaces;
    dev->ifaces = iface;
    return 0;
}

struct net_iface *net_device_get_iface(struct net_device *dev, int family) {
    struct net_iface *entry;
    for (entry = dev->ifaces; entry; entry = entry->next) {
        if (entry->family == family) {
            return entry;
        }
    }
    return NULL;
}
