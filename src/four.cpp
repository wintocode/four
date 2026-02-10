#include <new>
#include <math.h>
#include <string.h>
#include <distingnt/api.h>
#include "dsp.h"

// --- Algorithm struct ---

struct _fourAlgorithm : public _NT_algorithm
{
    // Oscillator state
    float phase[4];
    float prevOutput[4];     // Previous output for feedback

    // Cached parameter values (set by parameterChanged)
    float opLevel[4];        // 0.0-1.0
    float opFeedback[4];     // 0.0-1.0
    float opWarp[4];         // 0.0-1.0
    float opFold[4];         // 0.0-1.0
    uint8_t opFoldType[4];   // 0-2
    uint8_t opFreqMode[4];   // 0=ratio, 1=fixed
    float opCoarse[4];       // harmonic ratio (ratio mode) or Hz (fixed mode)
    float opFine[4];         // multiplier from cents

    float xm;                // 0.0-1.0
    float globalVCA;         // 0.0-1.0
    float fineTune;          // multiplier from cents
    uint8_t algorithm;       // 0-7
    uint8_t oversample;      // 0=off, 1=2x
    uint8_t polyblep;        // 0=off, 1=on

    // MIDI state
    float baseFrequency;     // Hz, from V/OCT or MIDI
    float pitchBendFactor;   // multiplier (1.0 = no bend)
    uint8_t midiNote;        // current MIDI note number
    uint8_t midiGate;        // 1=on, 0=off
    uint8_t midiChannel;     // 0-15

    // Oversampling state
    float dsBuffer[2];       // Downsample filter state

    _fourAlgorithm()
    {
        memset( phase, 0, sizeof(phase) );
        memset( prevOutput, 0, sizeof(prevOutput) );
        for ( int i = 0; i < 4; ++i )
        {
            opLevel[i] = 1.0f;
            opFeedback[i] = 0.0f;
            opWarp[i] = 0.0f;
            opFold[i] = 0.0f;
            opFoldType[i] = 0;
            opFreqMode[i] = 0;
            opCoarse[i] = 1.0f;
            opFine[i] = 1.0f;
        }
        xm = 0.0f;
        globalVCA = 1.0f;
        fineTune = 1.0f;
        algorithm = 0;
        oversample = 0;
        polyblep = 0;
        baseFrequency = 261.63f;  // C4
        pitchBendFactor = 1.0f;
        midiNote = 60;
        midiGate = 0;
        midiChannel = 0;
        dsBuffer[0] = 0.0f;
        dsBuffer[1] = 0.0f;
    }
};

// --- Parameter indices ---

enum {
    // I/O
    kParamOutput,
    kParamOutputMode,

    // Global
    kParamAlgorithm,
    kParamXM,
    kParamFineTune,
    kParamOversampling,
    kParamPolyBLEP,
    kParamMidiChannel,
    kParamGlobalVCA,

    // Operator 1
    kParamOp1FreqMode,
    kParamOp1Coarse,
    kParamOp1Fine,
    kParamOp1Level,
    kParamOp1Feedback,
    kParamOp1Warp,
    kParamOp1Fold,
    kParamOp1FoldType,

    // Operator 2
    kParamOp2FreqMode,
    kParamOp2Coarse,
    kParamOp2Fine,
    kParamOp2Level,
    kParamOp2Feedback,
    kParamOp2Warp,
    kParamOp2Fold,
    kParamOp2FoldType,

    // Operator 3
    kParamOp3FreqMode,
    kParamOp3Coarse,
    kParamOp3Fine,
    kParamOp3Level,
    kParamOp3Feedback,
    kParamOp3Warp,
    kParamOp3Fold,
    kParamOp3FoldType,

    // Operator 4
    kParamOp4FreqMode,
    kParamOp4Coarse,
    kParamOp4Fine,
    kParamOp4Level,
    kParamOp4Feedback,
    kParamOp4Warp,
    kParamOp4Fold,
    kParamOp4FoldType,

    // CV Inputs
    kParamVOctCV,
    kParamXMCV,
    kParamFMCV,
    kParamSyncCV,
    kParamGlobalVCACV,
    kParamOp1AMCV,
    kParamOp2AMCV,
    kParamOp3AMCV,
    kParamOp4AMCV,
    kParamOp1PMCV,
    kParamOp2PMCV,
    kParamOp3PMCV,
    kParamOp4PMCV,
    kParamOp1WarpCV,
    kParamOp2WarpCV,
    kParamOp3WarpCV,
    kParamOp4WarpCV,
    kParamOp1FoldCV,
    kParamOp2FoldCV,
    kParamOp3FoldCV,
    kParamOp4FoldCV,

    kNumParams
};

// Helper: first param index for operator N (0-based)
static inline int opParam( int op, int offset ) { return kParamOp1FreqMode + op * 8 + offset; }
// Offsets within an operator block
enum {
    kOpFreqMode = 0,
    kOpCoarse   = 1,
    kOpFine     = 2,
    kOpLevel    = 3,
    kOpFeedback = 4,
    kOpWarp     = 5,
    kOpFold     = 6,
    kOpFoldType = 7,
};
// Helper: CV param index for operator N (0-based)
static inline int opAMCV( int op ) { return kParamOp1AMCV + op; }
static inline int opPMCV( int op ) { return kParamOp1PMCV + op; }
static inline int opWarpCV( int op ) { return kParamOp1WarpCV + op; }
static inline int opFoldCV( int op ) { return kParamOp1FoldCV + op; }

// --- Enum strings ---

static const char* algorithmStrings[] = { "1","2","3","4","5","6","7","8", NULL };
static const char* offOnStrings[]     = { "Off","On", NULL };
static const char* off2xStrings[]     = { "Off","2x", NULL };
static const char* freqModeStrings[]  = { "Ratio","Fixed", NULL };
static const char* foldTypeStrings[]  = { "Symmetric","Asymmetric","Soft Clip", NULL };

// --- Parameter definitions ---

// Macro for one operator's 8 parameters
#define OP_PARAMS(n) \
    { "Op" #n " Freq Mode", 0, 1, 0, kNT_unitEnum, 0, freqModeStrings }, \
    { "Op" #n " Coarse",    1, 32, 1, kNT_unitNone, 0, NULL }, \
    { "Op" #n " Fine",   -100, 100, 0, kNT_unitCents, 0, NULL }, \
    { "Op" #n " Level",     0, 100, 100, kNT_unitPercent, 0, NULL }, \
    { "Op" #n " Feedback",  0, 100, 0, kNT_unitPercent, 0, NULL }, \
    { "Op" #n " Warp",      0, 100, 0, kNT_unitPercent, 0, NULL }, \
    { "Op" #n " Fold",      0, 100, 0, kNT_unitPercent, 0, NULL }, \
    { "Op" #n " Fold Type", 0, 2, 0, kNT_unitEnum, 0, foldTypeStrings },

static _NT_parameter parameters[] = {
    // I/O
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE( "Output", 1, 13 )

    // Global
    { "Algorithm",    0,    7,   0,   kNT_unitEnum,    0, algorithmStrings },
    { "XM",           0,  100,   0,   kNT_unitPercent, 0, NULL },
    { "Fine Tune",  -100, 100,   0,   kNT_unitCents,   0, NULL },
    { "Oversampling", 0,    1,   0,   kNT_unitEnum,    0, off2xStrings },
    { "PolyBLEP",     0,    1,   0,   kNT_unitEnum,    0, offOnStrings },
    { "MIDI Channel", 1,   16,   1,   kNT_unitNone,    0, NULL },
    { "Global VCA",   0,  100, 100,   kNT_unitPercent, 0, NULL },

    // Operators
    OP_PARAMS(1)
    OP_PARAMS(2)
    OP_PARAMS(3)
    OP_PARAMS(4)

    // CV Inputs
    NT_PARAMETER_CV_INPUT( "V/OCT CV",       0, 0 )
    NT_PARAMETER_CV_INPUT( "XM CV",          0, 0 )
    NT_PARAMETER_CV_INPUT( "FM CV",          0, 0 )
    NT_PARAMETER_CV_INPUT( "Sync CV",        0, 0 )
    NT_PARAMETER_CV_INPUT( "Global VCA CV",  0, 0 )
    NT_PARAMETER_CV_INPUT( "Op1 AM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op2 AM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op3 AM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op4 AM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op1 PM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op2 PM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op3 PM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op4 PM CV",      0, 0 )
    NT_PARAMETER_CV_INPUT( "Op1 Warp CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Op2 Warp CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Op3 Warp CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Op4 Warp CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Op1 Fold CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Op2 Fold CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Op3 Fold CV",    0, 0 )
    NT_PARAMETER_CV_INPUT( "Op4 Fold CV",    0, 0 )
};

// --- Parameter pages ---

static const uint8_t pageIO[] = { kParamOutput, kParamOutputMode };
static const uint8_t pageGlobal[] = {
    kParamAlgorithm, kParamXM, kParamFineTune,
    kParamOversampling, kParamPolyBLEP, kParamGlobalVCA
};
static const uint8_t pageMIDI[] = { kParamMidiChannel };

#define OP_PAGE(n) \
    static const uint8_t pageOp##n[] = { \
        kParamOp##n##FreqMode, kParamOp##n##Coarse, kParamOp##n##Fine, \
        kParamOp##n##Level, kParamOp##n##Feedback, \
        kParamOp##n##Warp, kParamOp##n##Fold, kParamOp##n##FoldType \
    };
OP_PAGE(1) OP_PAGE(2) OP_PAGE(3) OP_PAGE(4)

static const uint8_t pageCVGlobal[] = {
    kParamVOctCV, kParamXMCV, kParamFMCV, kParamSyncCV, kParamGlobalVCACV
};
#define CV_PAGE(n) \
    static const uint8_t pageCVOp##n[] = { \
        kParamOp##n##AMCV, kParamOp##n##PMCV, \
        kParamOp##n##WarpCV, kParamOp##n##FoldCV \
    };
CV_PAGE(1) CV_PAGE(2) CV_PAGE(3) CV_PAGE(4)

static const _NT_parameterPage pages[] = {
    { .name = "I/O",        .numParams = ARRAY_SIZE(pageIO),        .params = pageIO },
    { .name = "Global",     .numParams = ARRAY_SIZE(pageGlobal),    .params = pageGlobal },
    { .name = "MIDI",       .numParams = ARRAY_SIZE(pageMIDI),      .params = pageMIDI },
    { .name = "Operator 1", .numParams = ARRAY_SIZE(pageOp1),       .params = pageOp1 },
    { .name = "Operator 2", .numParams = ARRAY_SIZE(pageOp2),       .params = pageOp2 },
    { .name = "Operator 3", .numParams = ARRAY_SIZE(pageOp3),       .params = pageOp3 },
    { .name = "Operator 4", .numParams = ARRAY_SIZE(pageOp4),       .params = pageOp4 },
    { .name = "CV Global",  .numParams = ARRAY_SIZE(pageCVGlobal),  .params = pageCVGlobal },
    { .name = "CV Op1",     .numParams = ARRAY_SIZE(pageCVOp1),     .params = pageCVOp1 },
    { .name = "CV Op2",     .numParams = ARRAY_SIZE(pageCVOp2),     .params = pageCVOp2 },
    { .name = "CV Op3",     .numParams = ARRAY_SIZE(pageCVOp3),     .params = pageCVOp3 },
    { .name = "CV Op4",     .numParams = ARRAY_SIZE(pageCVOp4),     .params = pageCVOp4 },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

// --- Lifecycle ---

static void calculateRequirements(
    _NT_algorithmRequirements& req,
    const int32_t* specifications )
{
    req.numParameters = ARRAY_SIZE(parameters);
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
    alg->parameterPages = &parameterPages;
    return alg;
}

// --- Parameter changed ---

static void parameterChanged( _NT_algorithm* self, int parameter )
{
    _fourAlgorithm* p = (_fourAlgorithm*)self;

    // Per-operator parameters
    for ( int op = 0; op < 4; ++op )
    {
        int base = kParamOp1FreqMode + op * 8;
        if ( parameter >= base && parameter < base + 8 )
        {
            int offset = parameter - base;
            switch ( offset )
            {
            case kOpFreqMode:
            {
                p->opFreqMode[op] = p->v[parameter];
                // Update coarse param range based on mode
                int coarseIdx = base + kOpCoarse;
                if ( p->opFreqMode[op] == 0 ) // Ratio
                {
                    parameters[coarseIdx].min = 1;
                    parameters[coarseIdx].max = 32;
                    parameters[coarseIdx].unit = kNT_unitNone;
                }
                else // Fixed
                {
                    parameters[coarseIdx].min = 1;
                    parameters[coarseIdx].max = 9999;
                    parameters[coarseIdx].unit = kNT_unitHz;
                }
                NT_updateParameterDefinition(
                    NT_algorithmIndex(self), coarseIdx );
                break;
            }
            case kOpCoarse:
                p->opCoarse[op] = (float)p->v[parameter];
                break;
            case kOpFine:
                // Convert cents to ratio multiplier: 2^(cents/1200)
                p->opFine[op] = exp2f( (float)p->v[parameter] / 1200.0f );
                break;
            case kOpLevel:
                p->opLevel[op] = (float)p->v[parameter] * 0.01f;
                break;
            case kOpFeedback:
                p->opFeedback[op] = (float)p->v[parameter] * 0.01f;
                break;
            case kOpWarp:
                p->opWarp[op] = (float)p->v[parameter] * 0.01f;
                break;
            case kOpFold:
                p->opFold[op] = (float)p->v[parameter] * 0.01f;
                break;
            case kOpFoldType:
                p->opFoldType[op] = p->v[parameter];
                break;
            }
            return;
        }
    }

    // Global parameters
    switch ( parameter )
    {
    case kParamAlgorithm:
        p->algorithm = p->v[parameter];
        break;
    case kParamXM:
        p->xm = (float)p->v[parameter] * 0.01f;
        break;
    case kParamFineTune:
        p->fineTune = exp2f( (float)p->v[parameter] / 1200.0f );
        break;
    case kParamOversampling:
        p->oversample = p->v[parameter];
        break;
    case kParamPolyBLEP:
        p->polyblep = p->v[parameter];
        break;
    case kParamMidiChannel:
        p->midiChannel = p->v[parameter] - 1;  // 1-16 → 0-15
        break;
    case kParamGlobalVCA:
        p->globalVCA = (float)p->v[parameter] * 0.01f;
        break;
    }
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
    .parameterChanged = parameterChanged,
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
