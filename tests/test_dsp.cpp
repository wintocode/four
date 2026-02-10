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

// --- Task 10: Wave Fold ---

TEST(fold_zero_is_passthrough)
{
    ASSERT_NEAR( four::wave_fold(0.5f, 0.0f, 0), 0.5f, 1e-6f );
    ASSERT_NEAR( four::wave_fold(-0.3f, 0.0f, 1), -0.3f, 1e-6f );
    ASSERT_NEAR( four::wave_fold(0.7f, 0.0f, 2), 0.7f, 1e-6f );
}

TEST(fold_symmetric)
{
    // Symmetric fold: signal folds back when exceeding ±1
    // With moderate fold, a 0.8 input driven to >1 should fold back
    float result = four::wave_fold( 0.8f, 0.5f, 0 );
    ASSERT( result >= -1.0f && result <= 1.0f );
}

TEST(fold_symmetric_stays_bounded)
{
    // Even with extreme fold, output stays in [-1, 1]
    for ( float input = -1.0f; input <= 1.0f; input += 0.1f )
    {
        float result = four::wave_fold( input, 1.0f, 0 );
        ASSERT( result >= -1.01f && result <= 1.01f );
    }
}

TEST(fold_asymmetric_stays_bounded)
{
    for ( float input = -1.0f; input <= 1.0f; input += 0.1f )
    {
        float result = four::wave_fold( input, 1.0f, 1 );
        ASSERT( result >= -1.01f && result <= 1.01f );
    }
}

TEST(fold_softclip_stays_bounded)
{
    for ( float input = -1.0f; input <= 1.0f; input += 0.1f )
    {
        float result = four::wave_fold( input, 1.0f, 2 );
        ASSERT( result >= -1.01f && result <= 1.01f );
    }
}

// --- Task 11: Self-Feedback ---

TEST(feedback_zero_amount)
{
    // No feedback → 0 contribution
    ASSERT_NEAR( four::calc_feedback(0.5f, 0.0f), 0.0f, 1e-6f );
}

TEST(feedback_full_amount)
{
    // Full feedback → soft-clipped previous output
    float result = four::calc_feedback( 0.8f, 1.0f );
    ASSERT( result > 0.0f && result <= 1.0f );
}

TEST(feedback_is_bounded)
{
    // Even with extreme previous output, feedback stays bounded
    float result = four::calc_feedback( 10.0f, 1.0f );
    ASSERT( result >= -1.0f && result <= 1.0f );
}

// --- Task 12: Algorithm Routing ---

TEST(algorithm_1_serial_chain)
{
    // Algo 1: 4→3→2→1, carrier: 1
    const four::Algorithm& a = four::algorithms[0];
    ASSERT( a.mod[3][2] );   // 4→3
    ASSERT( a.mod[2][1] );   // 3→2
    ASSERT( a.mod[1][0] );   // 2→1
    ASSERT( a.carrier[0] );
    ASSERT( !a.carrier[1] );
    ASSERT( !a.carrier[2] );
    ASSERT( !a.carrier[3] );
}

TEST(algorithm_5_two_pairs)
{
    // Algo 5: (4→3) + (2→1), carriers: {1, 3}
    const four::Algorithm& a = four::algorithms[4];
    ASSERT( a.mod[3][2] );   // 4→3
    ASSERT( a.mod[1][0] );   // 2→1
    ASSERT( a.carrier[0] );  // op1 carrier
    ASSERT( a.carrier[2] );  // op3 carrier
    ASSERT( !a.carrier[1] );
    ASSERT( !a.carrier[3] );
}

TEST(algorithm_8_all_carriers)
{
    // Algo 8: all carriers, no modulation
    const four::Algorithm& a = four::algorithms[7];
    for ( int i = 0; i < 4; ++i )
    {
        ASSERT( a.carrier[i] );
        for ( int j = 0; j < 4; ++j )
            ASSERT( !a.mod[i][j] );
    }
}

TEST(process_algorithm_8_sum)
{
    // All carriers: output = sum of all op levels
    float opOut[4] = { 0.5f, 0.3f, 0.2f, 0.1f };
    float level[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float result = four::sum_carriers( opOut, level, four::algorithms[7] );
    ASSERT_NEAR( result, 1.1f, 1e-6f );
}

TEST(process_algorithm_1_single_carrier)
{
    // Only op1 (idx 0) is carrier
    float opOut[4] = { 0.5f, 0.3f, 0.2f, 0.1f };
    float level[4] = { 0.8f, 1.0f, 1.0f, 1.0f };
    float result = four::sum_carriers( opOut, level, four::algorithms[0] );
    ASSERT_NEAR( result, 0.5f * 0.8f, 1e-6f );
}

TEST(gather_modulation)
{
    // For algo 1, op1 (idx 0) should receive modulation from op2 (idx 1)
    float opOut[4] = { 0.0f, 0.7f, 0.0f, 0.0f };
    float level[4] = { 1.0f, 0.5f, 1.0f, 1.0f };
    float xm = 0.8f;
    float pm = four::gather_modulation( 0, opOut, level, xm, four::algorithms[0] );
    // Expected: opOut[1] * level[1] * xm = 0.7 * 0.5 * 0.8 = 0.28
    ASSERT_NEAR( pm, 0.28f, 1e-5f );
}

// --- Task 16: 2x Oversampling ---

TEST(downsample_2x)
{
    // Simple half-band filter: average of two samples
    float s0 = 0.8f;
    float s1 = 0.6f;
    float result = four::downsample_2x( s0, s1 );
    ASSERT_NEAR( result, 0.7f, 1e-6f );
}

// --- Task 17: PolyBLEP Anti-Aliasing ---

TEST(polyblep_correction_near_zero)
{
    // Near phase discontinuity (phase ≈ 0 or 1), correction is non-zero
    float dt = 440.0f / 48000.0f;  // phase increment
    float correction = four::polyblep( 0.001f, dt );
    ASSERT( fabsf(correction) > 0.0f );
}

TEST(polyblep_correction_far_from_edge)
{
    // Far from discontinuity, correction is zero
    float dt = 440.0f / 48000.0f;
    float correction = four::polyblep( 0.5f, dt );
    ASSERT_NEAR( correction, 0.0f, 1e-6f );
}

TEST(polyblep_saw_reduces_aliasing)
{
    // A PolyBLEP-corrected saw should have less energy at Nyquist
    // Simple check: the transition region is smoother
    float dt = 440.0f / 48000.0f;
    float raw_transition = four::waveform_saw(0.999f) - four::waveform_saw(0.001f);
    // Raw saw jumps ~2 across the reset
    ASSERT( fabsf(raw_transition) > 1.5f );
    // After BLEP, the jump should be reduced
    float blep0 = four::waveform_saw(0.001f) - four::polyblep(0.001f, dt);
    float blep1 = four::waveform_saw(0.999f) - four::polyblep(0.999f, dt);
    float blep_transition = blep1 - blep0;
    ASSERT( fabsf(blep_transition) < fabsf(raw_transition) );
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
    run_fold_zero_is_passthrough();
    run_fold_symmetric();
    run_fold_symmetric_stays_bounded();
    run_fold_asymmetric_stays_bounded();
    run_fold_softclip_stays_bounded();
    run_feedback_zero_amount();
    run_feedback_full_amount();
    run_feedback_is_bounded();
    run_algorithm_1_serial_chain();
    run_algorithm_5_two_pairs();
    run_algorithm_8_all_carriers();
    run_process_algorithm_8_sum();
    run_process_algorithm_1_single_carrier();
    run_gather_modulation();
    run_downsample_2x();
    run_polyblep_correction_near_zero();
    run_polyblep_correction_far_from_edge();
    run_polyblep_saw_reduces_aliasing();

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return 0;
}
