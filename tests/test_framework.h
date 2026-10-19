#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TEST_ALL_RUNNER
extern int g_tests_run;
extern int g_tests_failed;
#else
static int g_tests_run = 0;
static int g_tests_failed = 0;
#endif

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (!(cond)) { \
        printf("  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
        g_tests_failed++; \
    } else { \
        printf("  [PASS] %s\n", msg); \
    } \
} while (0)

#define TEST_ASSERT_STR_EQ(a, b, msg) do { \
    g_tests_run++; \
    if (strcmp((a), (b)) != 0) { \
        printf("  [FAIL] %s:%d: %s (expected '%s', got '%s')\n", __FILE__, __LINE__, msg, (b), (a)); \
        g_tests_failed++; \
    } else { \
        printf("  [PASS] %s\n", msg); \
    } \
} while (0)

#endif /* TEST_FRAMEWORK_H */
