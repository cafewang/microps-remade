#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "platform.h"

#include "util.h"
#include "net.h"

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
    return pthread_kill(tid, irq);
}

static void *
intr_thread(void *arg)
{
    int terminate = 0, sig, err;
    struct irq_entry *entry;

    debugf("interrupt thread started, tid=%lu", pthread_self());
    pthread_barrier_wait(&barrier);

    while (!terminate) {
        err = sigwait(&sigmask, &sig);
        if (err) {
            errorf("sigwait() failure, err=%s", strerror(err));
            break;
        }
        debugf("signal received, sig=%d", sig);
        if (sig == SIGHUP) {
            debugf("interrupt thread terminating");
            terminate = 1;
            continue;
        }
        if (sig == SIGUSR1) {
            debugf("handling softirq");
            net_softirq_handler();
            continue;
        }
        for (entry = irqs; entry; entry = entry->next) {
            if (entry->irq == (unsigned int)sig) {
                debugf("invoking handler for irq=%u, name=%s", entry->irq, entry->name);
                entry->handler(entry->irq, entry->dev);
            }
        }
    }
    debugf("interrupt thread exited");
    return NULL;
}

int
intr_run(void)
{
    // block signals in the main thread
    int err = pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
    if (err) {
        // strerror illustrates the error code
        errorf("pthread_sigmask() failure, err=%s", strerror(err));
        return -1;
    }
    // create interrupt handling thread
    err = pthread_create(&tid, NULL, intr_thread, NULL);
    if (err) {
        errorf("pthread_create() failure, err=%s", strerror(err));
        return -1;
    }
    // wait for the thread to be ready
    pthread_barrier_wait(&barrier);
    return 0;
}

void
intr_shutdown(void)
{
    if (pthread_equal(tid, pthread_self())) {
        // intr_thread not created
        return;
    }   
    // send a sighup signal to interrupt thread
    pthread_kill(tid, SIGHUP);
    pthread_join(tid, NULL);
}

int
intr_init(void)
{
    tid = pthread_self();
    // set countdown to 2
    pthread_barrier_init(&barrier, NULL, 2);
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGHUP);
    sigaddset(&sigmask, SIGUSR1);
    return 0;
}
