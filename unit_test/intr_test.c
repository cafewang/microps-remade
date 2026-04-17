#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "platform.h"
#include "unit_test.h"

int get_thread_count() {
    FILE *fp;
    char path[1035];
    int count = 0;

    int pid = getpid();
    char cmd[1024];

    snprintf(cmd, sizeof(cmd), "ls -1 /proc/%d/task/ | wc -l", pid);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    if (fgets(path, sizeof(path), fp) != NULL) {
        count = atoi(path);
    }

    pclose(fp);
    return count;
}

void test_sig_mask() {
    intr_init();
    sigset_t *sigset = get_intr_sigset();
    ASSERT_EQ(1, sigismember(sigset, SIGHUP));
    ASSERT_EQ(1, sigismember(sigset, SIGUSR1));
    TEST_PASS;
}

void test_thread_count() {
    ASSERT_EQ(0, intr_run());
    ASSERT_EQ(2, get_thread_count());
    TEST_PASS;
}

int main(int argc, char *argv[]) {
    test_sig_mask();
    test_thread_count();
    return 0;
}
