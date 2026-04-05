#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "unit_test.h"

void test_sig_mask() {
    intr_init();
    sigset_t *sigset = get_intr_sigset();
    ASSERT_EQ(1, sigismember(sigset, SIGHUP));
    ASSERT_EQ(1, sigismember(sigset, SIGUSR1));
    TEST_PASS;
}

int main(int argc, char *argv[]) {
    test_sig_mask();
    return 0;
}
