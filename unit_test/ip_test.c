#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "net.h"
#include "ip.h"
#include "unit_test.h"

void test_address_conversion(void) {
    const char *ip_str = "192.168.1.3";
    ip_addr_t addr;

    if (ip_addr_pton(ip_str, &addr) == -1) {
        fprintf(stderr, "ip_addr_pton() failed\n");
        return;
    }
    ASSERT_EQ(192, *(uint8_t*)(&addr));
    ASSERT_EQ(168, *((uint8_t*)(&addr) + 1));
    ASSERT_EQ(1, *((uint8_t*)(&addr) + 2));
    ASSERT_EQ(3, *((uint8_t*)(&addr) + 3));

    char buf[IP_ADDR_STR_LEN];
    ip_addr_ntop(addr, buf, sizeof(buf));
    ASSERT_EQ(0, strcmp(ip_str, buf));
    TEST_PASS;
}

void test_wrong_ip_format() {
    ip_addr_t addr;
    ASSERT_EQ(-1, ip_addr_pton("192.168.1.3.4", &addr));
    ASSERT_EQ(-1, ip_addr_pton("192.168.1.300", &addr));
    ASSERT_EQ(-1, ip_addr_pton("192.168.1.abc", &addr));
    TEST_PASS;
}

void test_check_sum(void) {
    uint16_t ip_hdr[2] = { hton16(0x0002), hton16(0x0001) };
    // 0x0002 + 0x0001 => 0x0003 inversion => 0xfffc
    // length is in bytes, so we use 4 (2 uint16_t)
    uint16_t sum = cksum16((uint16_t*)&ip_hdr, 4, 0);
    ASSERT_EQ(0xff, *(uint8_t*)(&sum));
    ASSERT_EQ(0xfc, *((uint8_t*)(&sum) + 1));
    ASSERT_EQ(0, cksum16((uint16_t*)&ip_hdr, 4, sum));
    TEST_PASS;
}

int main(int argc, char *argv[]) {
    test_address_conversion();
    test_wrong_ip_format();
    test_check_sum();
    return 0;
}
