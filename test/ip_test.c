#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include "util.h"
#include "net.h"
#include "ip.h"

#include "test.h"

void address_conversion_test(void) {
    const char *ip_str = "192.168.1.3";
    ip_addr_t addr;

    if (ip_addr_pton(ip_str, &addr) == -1) {
        fprintf(stderr, "ip_addr_pton() failed\n");
        return 1;
    }
    assert(192 == *(uint8_t*)(&addr));
    assert(168 == *((uint8_t*)(&addr) + 1));
    assert(1 == *((uint8_t*)(&addr) + 2));
    assert(3 == *((uint8_t*)(&addr) + 3));

    char buf[IP_ADDR_STR_LEN];
    ip_addr_ntop(addr, buf, sizeof(buf));
    assert(0 == strcmp(ip_str, buf));
}

void check_sum_test(void) {
    uint16_t ip_hdr[2] = { hton16(0x0002), hton16(0x0001) };
    // 0x0002 + 0x0001 => 0x0003 inversion => 0xfffc
    // length is in bytes, so we use 4 (2 uint16_t)
    uint16_t sum = cksum16((uint16_t*)&ip_hdr, 4, 0);
    assert(*(uint8_t*)(&sum) == 0xff);
    assert(*((uint8_t*)(&sum) + 1) == 0xfc);
    assert(0 == cksum16((uint16_t*)&ip_hdr, 4, sum));
}

int main(int argc, char *argv[]) {
    address_conversion_test();
    check_sum_test();
    return 0;
}
