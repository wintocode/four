#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Test macros
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    static void test_##name(); \
    static void run_##name() { \
        tests_run++; \
        printf("  %s ... ", #name); \
        test_##name(); \
        tests_passed++; \
        printf("PASS\n"); \
    } \
    static void test_##name()

#define ASSERT(cond) \
    do { if (!(cond)) { \
        printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } } while(0)

#define ASSERT_NEAR(a, b, eps) \
    do { float _a=(a), _b=(b); if (fabsf(_a-_b) > (eps)) { \
        printf("FAIL\n    %s:%d: %f != %f (eps=%f)\n", \
               __FILE__, __LINE__, _a, _b, (float)(eps)); \
        exit(1); \
    } } while(0)

#include "../src/dsp.h"

// --- Tests will be added here as DSP functions are implemented ---

TEST(placeholder)
{
    ASSERT(1 + 1 == 2);
}

// --- Runner ---

int main()
{
    printf("Four DSP tests:\n");

    run_placeholder();

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return 0;
}
