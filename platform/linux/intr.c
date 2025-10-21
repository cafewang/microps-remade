#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "platform.h"

#include "util.h"

struct irq_entry {
    struct irq_entry *next;
    unsigned int irq;
    intr_handler handler;
    int flags; // check whether INTR_IRQ_SHARED is set
    char name[16];
    void *dev; // device pointer, could be net_device or others
};

/* NOTE: if you want to add/delete the entries after intr_run(), you need to protect these lists with a mutex. */
static struct irq_entry *irqs;

static sigset_t sigmask;

static pthread_t tid;
static pthread_barrier_t barrier;

int
intr_request_irq(unsigned int irq, intr_handler handler, int flags, const char *name, void *dev)
{
    struct irq_entry *entry;

    debugf("irq=%u, handler=%p, flags=0x%04x, name=%s, dev=%p", irq, handler, flags, name, dev);
    for (entry = irqs; entry; entry = entry->next) {
        if (entry->irq == irq) {
            if (entry->flags ^ INTR_IRQ_SHARED || flags ^ INTR_IRQ_SHARED) {
                errorf("irq already registered, irq=%u, name=%s", irq, entry->name);
                return -1;
            }
        }
    }

    struct irq_entry *new_entry;
    new_entry = memory_alloc(sizeof(*new_entry));
    if (!new_entry) {
        errorf("memory_alloc() failure");
        return -1;
    }
    init_irq_entry(new_entry, irq, handler, flags, name, dev);
    new_entry->next = irqs;
    irqs = new_entry;
    sigaddset(&sigmask, irq);
    debugf("irq registered, irq=%u, name=%s", irq, name);
    return 0;
}

inline int init_irq_entry(struct irq_entry *entry, unsigned int irq, intr_handler handler, int flags, const char *name, void *dev) {
    entry->irq = irq;
    entry->handler = handler;
    entry->flags = flags;
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->dev = dev;
}

int
intr_raise_irq(unsigned int irq)
{
}

static void *
intr_thread(void *arg)
{
}

int
intr_run(void)
{
}

void
intr_shutdown(void)
{
}

int
intr_init(void)
{
}
