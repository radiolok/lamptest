#ifndef CHECK_H
#define CHECK_H

// Minimal test harness: CHECK* record failures and keep going,
// RUN_TEST resets the firmware state before each test.

#include <stdio.h>
#include <string.h>

extern int check_failures;
extern int check_count;

#define CHECK(cond) do {                                                    \
    check_count++;                                                          \
    if (!(cond)) {                                                          \
        check_failures++;                                                   \
        printf("%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);     \
    }                                                                       \
} while (0)

#define CHECK_EQ(actual, expected) do {                                     \
    long long a_ = (long long)(actual), e_ = (long long)(expected);         \
    check_count++;                                                          \
    if (a_ != e_) {                                                         \
        check_failures++;                                                   \
        printf("%s:%d: %s == %lld, expected %s == %lld\n", __FILE__,        \
               __LINE__, #actual, a_, #expected, e_);                       \
    }                                                                       \
} while (0)

// Compare n characters of a non-terminated buffer with a string literal
#define CHECK_MEM(actual, expected) do {                                    \
    const char *e_ = (expected);                                            \
    size_t n_ = strlen(e_);                                                 \
    check_count++;                                                          \
    if (memcmp((actual), e_, n_) != 0) {                                    \
        check_failures++;                                                   \
        printf("%s:%d: %s == \"%.*s\", expected \"%s\"\n", __FILE__,        \
               __LINE__, #actual, (int)n_, (const char *)(actual), e_);     \
    }                                                                       \
} while (0)

void test_reset(void);

#define RUN_TEST(fn) do { test_reset(); fn(); } while (0)

int check_summary(const char *suite);

#endif
