#ifndef UNIT_TEST_H
#define UNIT_TEST_H

#include <stdio.h>

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "Assertion failed in function %s at %s:%d: %s == %s\n", __func__, __FILE__, __LINE__, #a, #b); \
        fprintf(stderr, "  Actual values: %d != %d\n", (a), (b)); \
        exit(1); \
    } \
} while (0)

#endif