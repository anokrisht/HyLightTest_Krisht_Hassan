/* Minimal Unity-compatible test helpers (subset) for CI host tests */
#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <stdlib.h>

extern int UnityTestsRun;

#define TEST_ASSERT_EQUAL_INT(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAIL: %s:%d: expected %d but got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        exit(1); \
    } \
} while(0)

#define RUN_TEST(fn) do { \
    printf("RUN: %s\n", #fn); \
    fn(); \
    UnityTestsRun++; \
} while(0)

#define UNITY_BEGIN() do { UnityTestsRun = 0; } while(0)
#define UNITY_END() do { printf("Tests run: %d\n", UnityTestsRun); } while(0)

#endif // UNITY_H
