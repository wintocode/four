#ifndef FOUR_DSP_H
#define FOUR_DSP_H

// Pure DSP functions for Four FM synthesizer.
// No Disting NT API dependencies — testable on desktop.

#include <math.h>

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

} // namespace four

#endif // FOUR_DSP_H
