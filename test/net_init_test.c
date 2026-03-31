#include <stdio.h>
#include <stddef.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>

#include "util.h"
#include "net.h"
#include "ip.h"

#include "driver/loopback.h"

#include "test.h"

static volatile sig_atomic_t terminate;

static void
on_signal(int s)
{
    (void)s;
    terminate = 1;
}

int get_thread_count() {
    int thread_count = 0;
    struct dirent *entry;
    DIR *dp = opendir("/proc/self/task");

    if (dp == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_DIR &&
            entry->d_name[0] != '.') {
            debugf("thread: %s", entry->d_name);
            thread_count++;
        }
    }
    closedir(dp);
    return thread_count;
}

static int
setup(void)
{
    struct net_device *dev;
    struct ip_iface *iface;

    signal(SIGINT, on_signal);
    if (net_init() == -1) {
        errorf("net_init() failure");
        return -1;
    }
    dev = loopback_init();
    if (!dev) {
        errorf("loopback_init() failure");
        return -1;
    }
    iface = ip_iface_alloc(LOOPBACK_IP_ADDR, LOOPBACK_NETMASK);
    if (!iface) {
        errorf("ip_iface_alloc() failure");
        return -1;
    }
    if (ip_iface_register(dev, iface) == -1) {
        errorf("ip_iface_register() failure");
        return -1;
    }
    if (net_run() == -1) {
        errorf("net_run() failure");
        return -1;
    }
    return 0;
}

static void
cleanup(void)
{
    net_shutdown();
}

int
main(int argc, char *argv[])
{
    if (setup() == -1) {
        errorf("setup() failure");
        return -1;
    }
    // test thread count equals 2 (main thread and interrupt thread)
    assert(get_thread_count() == 2);
    cleanup();
    return 0;
}
