#ifndef FOUR_DSP_H
#define FOUR_DSP_H

// Pure DSP functions for Four FM synthesizer.
// No Disting NT API dependencies — testable on desktop.

#include <math.h>
#include <stdint.h>

namespace four {

static constexpr float TWO_PI = 6.283185307179586f;

// Compute sine from normalized phase [0, 1)
inline float oscillator_sine( float phase )
{
    return sinf( phase * TWO_PI );
}

// Advance phase by increment, wrap to [0, 1)
inline void phase_advance( float& phase, float increment )
{
    phase += increment;
    phase -= floorf( phase );
}

// Frequency in ratio mode: base_hz * coarse_ratio * fine_multiplier
inline float calc_frequency_ratio( float base_hz, float coarse, float fine_mult )
{
    return base_hz * coarse * fine_mult;
}

// Frequency in fixed mode: coarse_hz * fine_multiplier
inline float calc_frequency_fixed( float coarse_hz, float fine_mult )
{
    return coarse_hz * fine_mult;
}

// V/OCT to frequency. 0V = C4 (261.63Hz), 1V/octave.
inline float voct_to_freq( float voltage )
{
    return 261.63f * exp2f( voltage );
}

// MIDI note to frequency. Note 69 = A4 = 440Hz.
inline float midi_note_to_freq( uint8_t note )
{
    return 440.0f * exp2f( ( (float)note - 69.0f ) / 12.0f );
}

// Raw waveform generators from normalized phase [0, 1)
inline float waveform_triangle( float phase )
{
    if ( phase < 0.25f )
        return phase * 4.0f;
    else if ( phase < 0.75f )
        return 2.0f - phase * 4.0f;
    else
        return phase * 4.0f - 4.0f;
}

inline float waveform_saw( float phase )
{
    return 2.0f * phase - 1.0f;
}

inline float waveform_pulse( float phase )
{
    return phase < 0.5f ? 1.0f : -1.0f;
}

// Wave warp: morph sine → triangle → saw → pulse
// phase: normalized [0, 1), warp: 0.0-1.0
inline float wave_warp( float phase, float warp )
{
    if ( warp <= 0.0f )
        return oscillator_sine( phase );

    float sine = oscillator_sine( phase );

    if ( warp <= 1.0f / 3.0f )
    {
        float t = warp * 3.0f;
        float tri = waveform_triangle( phase );
        return sine + t * ( tri - sine );
    }
    else if ( warp <= 2.0f / 3.0f )
    {
        float t = ( warp - 1.0f / 3.0f ) * 3.0f;
        float tri = waveform_triangle( phase );
        float saw = waveform_saw( phase );
        return tri + t * ( saw - tri );
    }
    else
    {
        float t = ( warp - 2.0f / 3.0f ) * 3.0f;
        float saw = waveform_saw( phase );
        float pls = waveform_pulse( phase );
        return saw + t * ( pls - saw );
    }
}

// Soft clipping function (tanh approximation, fast)
inline float soft_clip( float x )
{
    if ( x < -3.0f ) return -1.0f;
    if ( x >  3.0f ) return  1.0f;
    float x2 = x * x;
    return x * ( 27.0f + x2 ) / ( 27.0f + 9.0f * x2 );
}

// Symmetric fold: sin-based fold that wraps signal back within [-1, 1]
inline float fold_symmetric( float x )
{
    return sinf( x * 1.5707963f );  // sin(x * π/2)
}

// Asymmetric fold: positive folds, negative clips
inline float fold_asymmetric( float x )
{
    if ( x >= 0.0f )
        return sinf( x * 1.5707963f );
    else
        return soft_clip( x );
}

// Wave fold: applies drive based on fold amount, then folds
// input: signal [-1, 1], amount: 0.0-1.0, type: 0=sym, 1=asym, 2=soft
inline float wave_fold( float input, float amount, int type )
{
    if ( amount <= 0.0f )
        return input;

    // Drive: scale input by 1 + amount * 4 (up to 5× drive at max)
    float driven = input * ( 1.0f + amount * 4.0f );

    switch ( type )
    {
    case 0:  return fold_symmetric( driven );
    case 1:  return fold_asymmetric( driven );
    case 2:  return soft_clip( driven );
    default: return soft_clip( driven );
    }
}

struct Algorithm
{
    bool mod[4][4];     // mod[src][dst]: src modulates dst
    bool carrier[4];    // carrier[op]: outputs to mix
};

// 8 DX9-style algorithms (0-indexed)
static const Algorithm algorithms[8] = {
    // Algo 1: 4→3→2→1, carriers: {1}
    { { {0,0,0,0}, {1,0,0,0}, {0,1,0,0}, {0,0,1,0} },
      {true, false, false, false} },

    // Algo 2: (3+4)→2→1, carriers: {1}
    { { {0,0,0,0}, {1,0,0,0}, {0,1,0,0}, {0,1,0,0} },
      {true, false, false, false} },

    // Algo 3: (4→2→1) + (3→1), carriers: {1}
    { { {0,0,0,0}, {1,0,0,0}, {1,0,0,0}, {0,1,0,0} },
      {true, false, false, false} },

    // Algo 4: (4→3→1) + (2→1), carriers: {1}
    { { {0,0,0,0}, {1,0,0,0}, {1,0,0,0}, {0,0,1,0} },
      {true, false, false, false} },

    // Algo 5: (4→3) + (2→1), carriers: {1, 3}
    { { {0,0,0,0}, {1,0,0,0}, {0,0,0,0}, {0,0,1,0} },
      {true, false, true, false} },

    // Algo 6: 4→(1,2,3), carriers: {1, 2, 3}
    { { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {1,1,1,0} },
      {true, true, true, false} },

    // Algo 7: (4→3) + 2 + 1, carriers: {1, 2, 3}
    { { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,1,0} },
      {true, true, true, false} },

    // Algo 8: 1+2+3+4, carriers: all
    { { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },
      {true, true, true, true} },
};

// Gather phase modulation for a target operator from all sources
inline float gather_modulation(
    int target,
    const float opOut[4],
    const float level[4],
    float xm,
    const Algorithm& algo )
{
    float pm = 0.0f;
    for ( int src = 0; src < 4; ++src )
    {
        if ( algo.mod[src][target] )
            pm += opOut[src] * level[src] * xm;
    }
    return pm;
}

// Sum carrier outputs
inline float sum_carriers(
    const float opOut[4],
    const float level[4],
    const Algorithm& algo )
{
    float mix = 0.0f;
    for ( int op = 0; op < 4; ++op )
    {
        if ( algo.carrier[op] )
            mix += opOut[op] * level[op];
    }
    return mix;
}

// Calculate feedback contribution from previous output
// prev_output: previous sample output, amount: 0.0-1.0
// Returns phase modulation amount (bounded)
inline float calc_feedback( float prev_output, float amount )
{
    return soft_clip( prev_output * amount );
}

// Simple 2× downsampler (half-band average)
// s0: first sample (even), s1: second sample (odd)
inline float downsample_2x( float s0, float s1 )
{
    return ( s0 + s1 ) * 0.5f;
}

} // namespace four

#endif // FOUR_DSP_H
