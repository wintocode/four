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

// --- Task 7: Phase Accumulator + Sine ---

TEST(oscillator_sine_zero_phase)
{
    // Phase 0 → sin(0) = 0
    ASSERT_NEAR( four::oscillator_sine(0.0f), 0.0f, 1e-6f );
}

TEST(oscillator_sine_quarter)
{
    // Phase 0.25 → sin(π/2) = 1
    ASSERT_NEAR( four::oscillator_sine(0.25f), 1.0f, 1e-6f );
}

TEST(oscillator_sine_half)
{
    // Phase 0.5 → sin(π) = 0
    ASSERT_NEAR( four::oscillator_sine(0.5f), 0.0f, 1e-6f );
}

TEST(phase_advance)
{
    // Phase advances by freq/sampleRate per sample
    float phase = 0.0f;
    float inc = 440.0f / 48000.0f;
    four::phase_advance( phase, inc );
    ASSERT_NEAR( phase, inc, 1e-9f );
}

TEST(phase_advance_wraps)
{
    float phase = 0.999f;
    four::phase_advance( phase, 0.01f );
    ASSERT( phase >= 0.0f && phase < 1.0f );
    ASSERT_NEAR( phase, 0.009f, 1e-6f );
}

// --- Runner ---

int main()
{
    printf("Four DSP tests:\n");

    run_placeholder();
    run_oscillator_sine_zero_phase();
    run_oscillator_sine_quarter();
    run_oscillator_sine_half();
    run_phase_advance();
    run_phase_advance_wraps();

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return 0;
}
