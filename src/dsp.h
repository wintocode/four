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

} // namespace four

#endif // FOUR_DSP_H
