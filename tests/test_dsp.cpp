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

// --- Task 8: Frequency Calculation ---

TEST(freq_ratio_mode)
{
    // Ratio mode: base * coarse * fine_cents_multiplier
    // Base 440Hz, coarse 2, fine 0 cents → 880Hz
    float f = four::calc_frequency_ratio( 440.0f, 2.0f, 1.0f );
    ASSERT_NEAR( f, 880.0f, 0.01f );
}

TEST(freq_ratio_with_fine)
{
    // Base 440Hz, coarse 1, fine +100 cents → 440 * 2^(100/1200)
    float fine = exp2f( 100.0f / 1200.0f );
    float f = four::calc_frequency_ratio( 440.0f, 1.0f, fine );
    ASSERT_NEAR( f, 440.0f * fine, 0.01f );
}

TEST(freq_fixed_mode)
{
    // Fixed mode: coarse_hz * fine_cents_multiplier
    float f = four::calc_frequency_fixed( 1000.0f, 1.0f );
    ASSERT_NEAR( f, 1000.0f, 0.01f );
}

TEST(voct_to_freq)
{
    // 0V = C4 (261.63 Hz), 1V = C5 (523.25 Hz)
    ASSERT_NEAR( four::voct_to_freq(0.0f), 261.63f, 0.5f );
    ASSERT_NEAR( four::voct_to_freq(1.0f), 523.25f, 0.5f );
}

TEST(midi_note_to_freq)
{
    // MIDI 69 = A4 = 440Hz
    ASSERT_NEAR( four::midi_note_to_freq(69), 440.0f, 0.01f );
    // MIDI 60 = C4 = 261.63Hz
    ASSERT_NEAR( four::midi_note_to_freq(60), 261.63f, 0.5f );
}

// --- Task 9: Wave Warp ---

TEST(warp_zero_is_passthrough)
{
    // Warp=0 returns original value unchanged (sine passthrough)
    for ( float ph = 0.0f; ph < 1.0f; ph += 0.1f )
    {
        float sine = four::oscillator_sine(ph);
        ASSERT_NEAR( four::wave_warp(ph, 0.0f), sine, 1e-5f );
    }
}

TEST(warp_triangle)
{
    // Warp≈0.333: triangle wave
    // Triangle at phase 0 = 0, phase 0.25 = 1, phase 0.5 = 0, phase 0.75 = -1
    float w = 1.0f / 3.0f;
    ASSERT_NEAR( four::wave_warp(0.0f,  w), 0.0f,  0.15f );
    ASSERT_NEAR( four::wave_warp(0.25f, w), 1.0f,  0.15f );
    ASSERT_NEAR( four::wave_warp(0.5f,  w), 0.0f,  0.15f );
    ASSERT_NEAR( four::wave_warp(0.75f, w), -1.0f, 0.15f );
}

TEST(warp_saw)
{
    // Warp≈0.667: sawtooth. Rises from -1 to +1 then resets.
    float w = 2.0f / 3.0f;
    // Near start: close to -1
    ASSERT( four::wave_warp(0.01f, w) < -0.5f );
    // Middle: close to 0
    ASSERT_NEAR( four::wave_warp(0.5f, w), 0.0f, 0.15f );
    // Near end: close to +1
    ASSERT( four::wave_warp(0.99f, w) > 0.5f );
}

TEST(warp_pulse)
{
    // Warp=1.0: pulse/square. +1 for first half, -1 for second half.
    ASSERT( four::wave_warp(0.25f, 1.0f) > 0.9f );
    ASSERT( four::wave_warp(0.75f, 1.0f) < -0.9f );
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
    run_freq_ratio_mode();
    run_freq_ratio_with_fine();
    run_freq_fixed_mode();
    run_voct_to_freq();
    run_midi_note_to_freq();
    run_warp_zero_is_passthrough();
    run_warp_triangle();
    run_warp_saw();
    run_warp_pulse();

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return 0;
}
