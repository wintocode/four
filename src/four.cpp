#include <new>
#include <math.h>
#include <string.h>
#include <distingnt/api.h>
#include "dsp.h"

// --- Algorithm struct ---

struct _fourAlgorithm : public _NT_algorithm
{
    _fourAlgorithm() {}
};

// --- Parameters ---

enum {
    kParamOutput,
    kParamOutputMode,
    kNumParams
};

static _NT_parameter parameters[] = {
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE( "Output", 1, 13 )
};

// --- Lifecycle ---

static void calculateRequirements(
    _NT_algorithmRequirements& req,
    const int32_t* specifications )
{
    req.numParameters = kNumParams;
    req.sram = sizeof( _fourAlgorithm );
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

static _NT_algorithm* construct(
    const _NT_algorithmMemoryPtrs& ptrs,
    const _NT_algorithmRequirements& req,
    const int32_t* specifications )
{
    _fourAlgorithm* alg = new ( ptrs.sram ) _fourAlgorithm();
    alg->parameters = parameters;
    return alg;
}

// --- Audio ---

static void step(
    _NT_algorithm* self,
    float* busFrames,
    int numFramesBy4 )
{
    _fourAlgorithm* pThis = (_fourAlgorithm*)self;
    int numFrames = numFramesBy4 * 4;

    float* out = busFrames + ( pThis->v[kParamOutput] - 1 ) * numFrames;
    bool replace = pThis->v[kParamOutputMode];

    for ( int i = 0; i < numFrames; ++i )
    {
        if ( replace )
            out[i] = 0.0f;
        // else: leave bus content (add mode, adding 0)
    }
}

// --- Factory ---

static const _NT_factory factory = {
    .guid = NT_MULTICHAR( 'F', 'o', 'u', 'r' ),
    .name = "Four",
    .description = "4-op FM synthesizer",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = NULL,
    .step = step,
    .draw = NULL,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = kNT_tagInstrument,
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
    .serialise = NULL,
    .deserialise = NULL,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = NULL,
};

// --- Entry point ---

extern "C"
uintptr_t pluginEntry( _NT_selector selector, uint32_t data )
{
    switch ( selector )
    {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)( ( data == 0 ) ? &factory : NULL );
    }
    return 0;
}
