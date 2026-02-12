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
    float opLevelCVDepth[4]; // 0.0-1.0
    float opPMCVDepth[4];    // 0.0-1.0
    float opWarpCVDepth[4];  // 0.0-1.0
    float opFoldCVDepth[4];  // 0.0-1.0
    float opFeedback[4];     // 0.0-1.0
    float opWarp[4];         // 0.0-1.0
    float opFold[4];         // 0.0-1.0
    uint8_t opFoldType[4];   // 0-2
    uint8_t opFreqMode[4];   // 0=ratio, 1=fixed
    float opCoarse[4];       // harmonic ratio (ratio mode)
    float opFixedHz[4];      // Hz (fixed mode)
    float opFine[4];         // multiplier from cents

    float xm;                // 0.0-1.0
    float globalVCA;         // 0.0-1.0
    float fineTune;          // multiplier from cents
    uint8_t algorithm;       // 0-10
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
            opLevelCVDepth[i] = 0.0f;
            opPMCVDepth[i] = 0.0f;
            opWarpCVDepth[i] = 0.0f;
            opFoldCVDepth[i] = 0.0f;
            opFeedback[i] = 0.0f;
            opWarp[i] = 0.0f;
            opFold[i] = 0.0f;
            opFoldType[i] = 0;
            opFreqMode[i] = 0;
            opCoarse[i] = 1.0f;
            opFixedHz[i] = 440.0f;
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
    kParamVersion,

    // Operator 1
    kParamOp1FreqMode,
    kParamOp1Coarse,
    kParamOp1FixedHz,
    kParamOp1Fine,
    kParamOp1Level,
    kParamOp1Feedback,
    kParamOp1Warp,
    kParamOp1Fold,
    kParamOp1FoldType,

    // Operator 2
    kParamOp2FreqMode,
    kParamOp2Coarse,
    kParamOp2FixedHz,
    kParamOp2Fine,
    kParamOp2Level,
    kParamOp2Feedback,
    kParamOp2Warp,
    kParamOp2Fold,
    kParamOp2FoldType,

    // Operator 3
    kParamOp3FreqMode,
    kParamOp3Coarse,
    kParamOp3FixedHz,
    kParamOp3Fine,
    kParamOp3Level,
    kParamOp3Feedback,
    kParamOp3Warp,
    kParamOp3Fold,
    kParamOp3FoldType,

    // Operator 4
    kParamOp4FreqMode,
    kParamOp4Coarse,
    kParamOp4FixedHz,
    kParamOp4Fine,
    kParamOp4Level,
    kParamOp4Feedback,
    kParamOp4Warp,
    kParamOp4Fold,
    kParamOp4FoldType,

    // Operator Level CV Depth
    kParamOp1LevelCVDepth,
    kParamOp2LevelCVDepth,
    kParamOp3LevelCVDepth,
    kParamOp4LevelCVDepth,

    // Operator PM CV Depth
    kParamOp1PMCVDepth,
    kParamOp2PMCVDepth,
    kParamOp3PMCVDepth,
    kParamOp4PMCVDepth,

    // Operator Warp CV Depth
    kParamOp1WarpCVDepth,
    kParamOp2WarpCVDepth,
    kParamOp3WarpCVDepth,
    kParamOp4WarpCVDepth,

    // Operator Fold CV Depth
    kParamOp1FoldCVDepth,
    kParamOp2FoldCVDepth,
    kParamOp3FoldCVDepth,
    kParamOp4FoldCVDepth,

    // CV Inputs
    kParamVOctCV,
    kParamXMCV,
    kParamFMCV,
    kParamSyncCV,
    kParamGlobalVCACV,
    kParamOp1LevelCV,
    kParamOp2LevelCV,
    kParamOp3LevelCV,
    kParamOp4LevelCV,
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
static inline int opParam( int op, int offset ) { return kParamOp1FreqMode + op * 9 + offset; }
// Offsets within an operator block
enum {
    kOpFreqMode = 0,
    kOpCoarse   = 1,
    kOpFixedHz  = 2,
    kOpFine     = 3,
    kOpLevel    = 4,
    kOpFeedback = 5,
    kOpWarp     = 6,
    kOpFold     = 7,
    kOpFoldType = 8,
};
// Helper: CV param index for operator N (0-based)
static inline int opLevelCV( int op ) { return kParamOp1LevelCV + op; }
static inline int opPMCV( int op ) { return kParamOp1PMCV + op; }
static inline int opWarpCV( int op ) { return kParamOp1WarpCV + op; }
static inline int opFoldCV( int op ) { return kParamOp1FoldCV + op; }

// --- Enum strings ---

static const char* algorithmStrings[] = {
    "4 => 3 => 2 => 1",
    "(3+4) => 2 => 1",
    "4 => 2 => 1, 3 => 1",
    "4 => 3 => 1, 2 => 1",
    "4 => 3, 2 => 1",
    "4 => (1, 2, 3)",
    "4 => 3, 2, 1",
    "1, 2, 3, 4",
    "4 => 3 => (1, 2)",
    "(3+4) => (1, 2)",
    "(2+3+4) => 1",
    NULL
};

// Ratio strings: 0.25, 0.5, 0.75, then 1.0-32.0 in 0.5 steps
static const char* ratioStrings[] = {
    "0.25", "0.5", "0.75",
    "1", "1.5", "2", "2.5", "3", "3.5", "4", "4.5", "5", "5.5", "6", "6.5", "7", "7.5", "8", "8.5", "9", "9.5",
    "10", "10.5", "11", "11.5", "12", "12.5", "13", "13.5", "14", "14.5", "15", "15.5", "16", "16.5", "17", "17.5",
    "18", "18.5", "19", "19.5", "20", "20.5", "21", "21.5", "22", "22.5", "23", "23.5", "24", "24.5", "25", "25.5",
    "26", "26.5", "27", "27.5", "28", "28.5", "29", "29.5", "30", "30.5", "31", "31.5", "32",
    NULL
};
static const char* offOnStrings[]     = { "Off","On", NULL };
static const char* off2xStrings[]     = { "Off","2x", NULL };
static const char* freqModeStrings[]  = { "Ratio","Fixed", NULL };
static const char* foldTypeStrings[]  = { "Symmetric","Asymmetric","Soft Clip", NULL };

static const char* versionStrings[] = { FOUR_VERSION, NULL };

// --- Parameter definitions ---

// Macro for one operator's 9 parameters
#define OP_PARAMS(n) \
    { "Op" #n " Freq Mode", 0, 1, 0, kNT_unitEnum, 0, freqModeStrings }, \
    { "Op" #n " Coarse",    0, 64, 3, kNT_unitEnum, 0, ratioStrings }, \
    { "Op" #n " Fixed Hz",  1, 9999, 440, kNT_unitHz, 0, NULL }, \
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
    { "Algorithm",    0,   10,   0,   kNT_unitEnum,    0, algorithmStrings },
    { "XM",           0,  100,   0,   kNT_unitPercent, 0, NULL },
    { "Fine Tune",  -100, 100,   0,   kNT_unitCents,   0, NULL },
    { "Oversampling", 0,    1,   0,   kNT_unitEnum,    0, off2xStrings },
    { "PolyBLEP",     0,    1,   0,   kNT_unitEnum,    0, offOnStrings },
    { "MIDI Channel", 1,   16,   1,   kNT_unitNone,    0, NULL },
    { "Global VCA",   0,  100, 100,   kNT_unitPercent, 0, NULL },

    // Version (read-only)
    { "Version",      0,    0,   0,   kNT_unitEnum,    0, versionStrings },

    // Operators
    OP_PARAMS(1)
    OP_PARAMS(2)
    OP_PARAMS(3)
    OP_PARAMS(4)

    // Operator Level CV Depth
    { "Op1 Level CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op2 Level CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op3 Level CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op4 Level CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },

    // Operator PM CV Depth
    { "Op1 PM CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op2 PM CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op3 PM CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op4 PM CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },

    // Operator Warp CV Depth
    { "Op1 Warp CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op2 Warp CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op3 Warp CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op4 Warp CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },

    // Operator Fold CV Depth
    { "Op1 Fold CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op2 Fold CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op3 Fold CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },
    { "Op4 Fold CV Depth", 0, 100, 0, kNT_unitPercent, 0, NULL },

    // CV Inputs
    NT_PARAMETER_CV_INPUT( "V/OCT CV",       0, 0 )
    NT_PARAMETER_CV_INPUT( "XM CV",          0, 0 )
    NT_PARAMETER_CV_INPUT( "FM CV",          0, 0 )
    NT_PARAMETER_CV_INPUT( "Sync CV",        0, 0 )
    NT_PARAMETER_CV_INPUT( "Global VCA CV",  0, 0 )
    NT_PARAMETER_CV_INPUT( "Op1 Level CV",   0, 0 )
    NT_PARAMETER_CV_INPUT( "Op2 Level CV",   0, 0 )
    NT_PARAMETER_CV_INPUT( "Op3 Level CV",   0, 0 )
    NT_PARAMETER_CV_INPUT( "Op4 Level CV",   0, 0 )
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
    kParamOversampling, kParamPolyBLEP, kParamGlobalVCA,
    kParamVersion
};
static const uint8_t pageMIDI[] = { kParamMidiChannel };

#define OP_PAGE(n) \
    static const uint8_t pageOp##n[] = { \
        kParamOp##n##FreqMode, kParamOp##n##Coarse, kParamOp##n##FixedHz, \
        kParamOp##n##Fine, kParamOp##n##Level, kParamOp##n##Feedback, \
        kParamOp##n##Warp, kParamOp##n##Fold, kParamOp##n##FoldType \
    };
OP_PAGE(1) OP_PAGE(2) OP_PAGE(3) OP_PAGE(4)

static const uint8_t pageCVGlobal[] = {
    kParamVOctCV, kParamXMCV, kParamFMCV, kParamSyncCV, kParamGlobalVCACV
};
#define CV_PAGE(n) \
    static const uint8_t pageCVOp##n[] = { \
        kParamOp##n##LevelCV, kParamOp##n##LevelCVDepth, \
        kParamOp##n##PMCV, kParamOp##n##PMCVDepth, \
        kParamOp##n##WarpCV, kParamOp##n##WarpCVDepth, \
        kParamOp##n##FoldCV, kParamOp##n##FoldCVDepth \
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

// --- MIDI CC mapping ---

// CC 14-72 → 59 value parameters (excludes CV bus selectors)
// 7 global + 36 per-op (9×4) + 16 CV depths (4×4)
static const int8_t ccToParam[128] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,                     // 0-13
    kParamAlgorithm, kParamXM, kParamFineTune,                      // 14-16
    kParamOversampling, kParamPolyBLEP, kParamMidiChannel,          // 17-19
    kParamGlobalVCA,                                                // 20
    kParamOp1FreqMode, kParamOp1Coarse, kParamOp1FixedHz,          // 21-23
    kParamOp1Fine, kParamOp1Level, kParamOp1Feedback,               // 24-26
    kParamOp1Warp, kParamOp1Fold, kParamOp1FoldType,                // 27-29
    kParamOp2FreqMode, kParamOp2Coarse, kParamOp2FixedHz,          // 30-32
    kParamOp2Fine, kParamOp2Level, kParamOp2Feedback,               // 33-35
    kParamOp2Warp, kParamOp2Fold, kParamOp2FoldType,                // 36-38
    kParamOp3FreqMode, kParamOp3Coarse, kParamOp3FixedHz,          // 39-41
    kParamOp3Fine, kParamOp3Level, kParamOp3Feedback,               // 42-44
    kParamOp3Warp, kParamOp3Fold, kParamOp3FoldType,                // 45-47
    kParamOp4FreqMode, kParamOp4Coarse, kParamOp4FixedHz,          // 48-50
    kParamOp4Fine, kParamOp4Level, kParamOp4Feedback,               // 51-53
    kParamOp4Warp, kParamOp4Fold, kParamOp4FoldType,                // 54-56
    kParamOp1LevelCVDepth, kParamOp2LevelCVDepth,                  // 57-58
    kParamOp3LevelCVDepth, kParamOp4LevelCVDepth,                  // 59-60
    kParamOp1PMCVDepth, kParamOp2PMCVDepth,                        // 61-62
    kParamOp3PMCVDepth, kParamOp4PMCVDepth,                        // 63-64
    kParamOp1WarpCVDepth, kParamOp2WarpCVDepth,                    // 65-66
    kParamOp3WarpCVDepth, kParamOp4WarpCVDepth,                    // 67-68
    kParamOp1FoldCVDepth, kParamOp2FoldCVDepth,                    // 69-70
    kParamOp3FoldCVDepth, kParamOp4FoldCVDepth,                    // 71-72
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 73-88
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 89-104
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,               // 105-120
    -1,-1,-1,-1,-1,-1,-1                                           // 121-127
};

// Scale CC value (0-127) to parameter's min..max range
static int16_t scaleCCToParam( uint8_t ccValue, int paramIndex )
{
    int16_t mn = parameters[paramIndex].min;
    int16_t mx = parameters[paramIndex].max;
    return mn + (int16_t)( (int32_t)ccValue * ( mx - mn ) / 127 );
}

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
        int base = kParamOp1FreqMode + op * 9;
        if ( parameter >= base && parameter < base + 9 )
        {
            int offset = parameter - base;
            switch ( offset )
            {
            case kOpFreqMode:
            {
                p->opFreqMode[op] = p->v[parameter];
                // Update coarse param unit display based on mode
                int coarseIdx = base + kOpCoarse;
                if ( p->opFreqMode[op] == 0 ) // Ratio
                {
                    parameters[coarseIdx].unit = kNT_unitEnum;
                }
                else // Fixed
                {
                    parameters[coarseIdx].unit = kNT_unitHz;
                }
                NT_updateParameterDefinition(
                    NT_algorithmIndex(self), coarseIdx );
                break;
            }
            case kOpCoarse:
            {
                // Ratio mode: convert enum index to float ratio
                if ( p->opFreqMode[op] == 0 ) // Ratio
                {
                    int idx = p->v[parameter];
                    if ( idx == 0 )
                        p->opCoarse[op] = 0.25f;
                    else if ( idx == 1 )
                        p->opCoarse[op] = 0.5f;
                    else if ( idx == 2 )
                        p->opCoarse[op] = 0.75f;
                    else
                        p->opCoarse[op] = (float)( idx - 1 ) * 0.5f;
                }
                // Fixed mode: handled by kOpFixedHz, ignore here
                break;
            }
            case kOpFixedHz:
            {
                // Fixed mode: Hz value
                p->opFixedHz[op] = (float)p->v[parameter];
                break;
            }
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

    // Operator Level CV Depth
    case kParamOp1LevelCVDepth:
        p->opLevelCVDepth[0] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp2LevelCVDepth:
        p->opLevelCVDepth[1] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp3LevelCVDepth:
        p->opLevelCVDepth[2] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp4LevelCVDepth:
        p->opLevelCVDepth[3] = (float)p->v[parameter] * 0.01f;
        break;

    // Operator PM CV Depth
    case kParamOp1PMCVDepth:
        p->opPMCVDepth[0] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp2PMCVDepth:
        p->opPMCVDepth[1] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp3PMCVDepth:
        p->opPMCVDepth[2] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp4PMCVDepth:
        p->opPMCVDepth[3] = (float)p->v[parameter] * 0.01f;
        break;

    // Operator Warp CV Depth
    case kParamOp1WarpCVDepth:
        p->opWarpCVDepth[0] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp2WarpCVDepth:
        p->opWarpCVDepth[1] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp3WarpCVDepth:
        p->opWarpCVDepth[2] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp4WarpCVDepth:
        p->opWarpCVDepth[3] = (float)p->v[parameter] * 0.01f;
        break;

    // Operator Fold CV Depth
    case kParamOp1FoldCVDepth:
        p->opFoldCVDepth[0] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp2FoldCVDepth:
        p->opFoldCVDepth[1] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp3FoldCVDepth:
        p->opFoldCVDepth[2] = (float)p->v[parameter] * 0.01f;
        break;
    case kParamOp4FoldCVDepth:
        p->opFoldCVDepth[3] = (float)p->v[parameter] * 0.01f;
        break;
    }
}

// --- Audio ---

static void step(
    _NT_algorithm* self,
    float* busFrames,
    int numFramesBy4 )
{
    _fourAlgorithm* p = (_fourAlgorithm*)self;
    int numFrames = numFramesBy4 * 4;

    float* out = busFrames + ( p->v[kParamOutput] - 1 ) * numFrames;
    bool replace = p->v[kParamOutputMode];

    int actualRate = p->oversample ? 2 : 1;
    float sampleRate = (float)NT_globals.sampleRate;
    float effectiveSampleRate = sampleRate * (float)actualRate;
    const four::Algorithm& algo = four::algorithms[p->algorithm];

    // Read CV buses (0 = not connected)
    const float* cvVOct     = p->v[kParamVOctCV]     ? busFrames + (p->v[kParamVOctCV] - 1) * numFrames     : NULL;
    const float* cvXM       = p->v[kParamXMCV]       ? busFrames + (p->v[kParamXMCV] - 1) * numFrames       : NULL;
    const float* cvFM       = p->v[kParamFMCV]       ? busFrames + (p->v[kParamFMCV] - 1) * numFrames       : NULL;
    const float* cvSync     = p->v[kParamSyncCV]     ? busFrames + (p->v[kParamSyncCV] - 1) * numFrames     : NULL;
    const float* cvGlobalVCA= p->v[kParamGlobalVCACV]? busFrames + (p->v[kParamGlobalVCACV] - 1) * numFrames: NULL;

    const float* cvLevel[4];
    const float* cvPM[4];
    const float* cvWarp[4];
    const float* cvFold[4];
    for ( int op = 0; op < 4; ++op )
    {
        int16_t bus;
        bus = p->v[opLevelCV(op)]; cvLevel[op] = bus ? busFrames + (bus-1)*numFrames : NULL;
        bus = p->v[opPMCV(op)];    cvPM[op]     = bus ? busFrames + (bus-1)*numFrames : NULL;
        bus = p->v[opWarpCV(op)];  cvWarp[op]   = bus ? busFrames + (bus-1)*numFrames : NULL;
        bus = p->v[opFoldCV(op)];  cvFold[op]   = bus ? busFrames + (bus-1)*numFrames : NULL;
    }

    // Sync state (edge detection)
    float prevSync = p->dsBuffer[1];

    // Pre-compute operator frequencies
    float opFreq[4];
    for ( int op = 0; op < 4; ++op )
    {
        float base = p->baseFrequency * p->pitchBendFactor * p->fineTune;
        if ( p->opFreqMode[op] == 0 )  // Ratio
            opFreq[op] = four::calc_frequency_ratio( base, p->opCoarse[op], p->opFine[op] );
        else  // Fixed
            opFreq[op] = four::calc_frequency_fixed( p->opFixedHz[op], p->opFine[op] );
    }

    for ( int i = 0; i < numFrames; ++i )
    {
        // --- Per-sample modulations ---

        // V/OCT: overridden by MIDI when gate is on
        float baseFreq = p->baseFrequency;
        if ( cvVOct && !p->midiGate )
            baseFreq = four::voct_to_freq( cvVOct[i] );

        // Recompute op frequencies if V/OCT varies per sample
        if ( cvVOct || cvFM )
        {
            for ( int op = 0; op < 4; ++op )
            {
                float base = baseFreq * p->pitchBendFactor * p->fineTune;
                float fm = cvFM ? cvFM[i] * 1000.0f : 0.0f;
                if ( p->opFreqMode[op] == 0 )
                    opFreq[op] = four::calc_frequency_ratio( base, p->opCoarse[op], p->opFine[op] ) + fm;
                else
                    opFreq[op] = four::calc_frequency_fixed( p->opCoarse[op], p->opFine[op] ) + fm;
                if ( opFreq[op] < 0.0f ) opFreq[op] = 0.0f;
            }
        }

        // Sync: reset all phases on rising edge
        if ( cvSync )
        {
            if ( cvSync[i] > 0.5f && prevSync <= 0.5f )
            {
                for ( int op = 0; op < 4; ++op )
                    p->phase[op] = 0.0f;
            }
            prevSync = cvSync[i];
        }

        // XM with CV
        float xm = p->xm;
        if ( cvXM )
            xm = fminf( 1.0f, fmaxf( 0.0f, xm + cvXM[i] * 0.2f ) );

        // Compute effective operator levels with CV modulation
        float effectiveLevel[4];
        for ( int op = 0; op < 4; ++op )
        {
            effectiveLevel[op] = p->opLevel[op];
            if ( cvLevel[op] )
            {
                // CV is ±1.0, scale by depth and 0.2 factor for useful range
                float cvContribution = cvLevel[op][i] * p->opLevelCVDepth[op] * 0.2f;
                effectiveLevel[op] += cvContribution;
                effectiveLevel[op] = fmaxf( 0.0f, fminf( 1.0f, effectiveLevel[op] ) );
            }
        }

        // --- Process operators with optional oversampling ---
        float outputSample = 0.0f;

        for ( int os = 0; os < actualRate; ++os )
        {
            float opOut[4];

            for ( int op = 3; op >= 0; --op )
            {
                // Phase increment at effective rate
                float inc = opFreq[op] / effectiveSampleRate;

                // Gather modulation from algorithm routing
                float pm = four::gather_modulation( op, opOut, effectiveLevel, xm, algo );

                // Self-feedback
                pm += four::calc_feedback( p->prevOutput[op], p->opFeedback[op] );

                // External PM CV
                if ( cvPM[op] )
                    pm += cvPM[op][i] * p->opPMCVDepth[op];

                // Advance phase
                four::phase_advance( p->phase[op], inc );

                // Compute waveform
                float modPhase = p->phase[op] + pm;
                modPhase -= floorf( modPhase );  // Wrap to [0, 1)

                // Warp amount with CV
                float warp = p->opWarp[op];
                if ( cvWarp[op] )
                    warp = fminf( 1.0f, fmaxf( 0.0f, warp + cvWarp[op][i] * p->opWarpCVDepth[op] * 0.2f ) );

                float sample;
                if ( warp > 0.0f )
                {
                    if ( p->polyblep )
                    {
                        float dt = opFreq[op] / effectiveSampleRate;
                        sample = four::wave_warp_blep( modPhase, warp, dt );
                    }
                    else
                    {
                        sample = four::wave_warp( modPhase, warp );
                    }
                }
                else
                    sample = four::oscillator_sine( modPhase );

                // Fold amount with CV
                float fold = p->opFold[op];
                if ( cvFold[op] )
                    fold = fminf( 1.0f, fmaxf( 0.0f, fold + cvFold[op][i] * p->opFoldCVDepth[op] * 0.2f ) );

                if ( fold > 0.0f )
                    sample = four::wave_fold( sample, fold, p->opFoldType[op] );

                opOut[op] = sample;
                p->prevOutput[op] = sample;
            }

            // --- Sum carriers ---
            float subSample = four::sum_carriers( opOut, effectiveLevel, algo );

            // Global VCA with CV
            float vca = p->globalVCA;
            if ( cvGlobalVCA )
                vca *= fmaxf( 0.0f, cvGlobalVCA[i] * 0.2f );

            subSample *= vca;

            if ( actualRate == 1 )
                outputSample = subSample;
            else if ( os == 0 )
                p->dsBuffer[0] = subSample;
            else
                outputSample = four::downsample_2x( p->dsBuffer[0], subSample );
        }

        // Output
        if ( replace )
            out[i] = outputSample;
        else
            out[i] += outputSample;
    }

    p->dsBuffer[1] = prevSync;  // Store sync state
}

// --- MIDI ---

static void midiMessage(
    _NT_algorithm* self,
    uint8_t byte0,
    uint8_t byte1,
    uint8_t byte2 )
{
    _fourAlgorithm* p = (_fourAlgorithm*)self;

    uint8_t status  = byte0 & 0xF0;
    uint8_t channel = byte0 & 0x0F;

    if ( channel != p->midiChannel )
        return;

    switch ( status )
    {
    case 0x90:  // Note On
        if ( byte2 > 0 )
        {
            p->midiNote = byte1;
            p->midiGate = 1;
            p->baseFrequency = four::midi_note_to_freq( byte1 );
        }
        else
        {
            // Velocity 0 = note off
            if ( byte1 == p->midiNote )
                p->midiGate = 0;
        }
        break;

    case 0x80:  // Note Off
        if ( byte1 == p->midiNote )
            p->midiGate = 0;
        break;

    case 0xB0:  // Control Change
    {
        int8_t paramIdx = ccToParam[byte1];
        if ( paramIdx >= 0 )
        {
            int16_t value = scaleCCToParam( byte2, paramIdx );
            NT_setParameterFromAudio( NT_algorithmIndex(self), paramIdx, value );
        }
        break;
    }

    case 0xE0:  // Pitch Bend
    {
        int16_t bend = ( (int16_t)byte2 << 7 ) | byte1;  // 0-16383
        float bendNorm = (float)( bend - 8192 ) / 8192.0f;  // -1 to +1
        // ±2 semitones pitch bend range
        p->pitchBendFactor = exp2f( bendNorm * 2.0f / 12.0f );
        break;
    }
    }
}

// --- Factory ---

static const _NT_factory factory = {
    .guid = NT_MULTICHAR( 'F', 'o', 'u', 'r' ),
    .name = "Four",
    .description = "Four v" FOUR_VERSION " - 4-op FM synthesizer",
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
    .midiMessage = midiMessage,
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

// --- Version ---

extern "C" const char* four_getVersion()
{
    return "Four v" FOUR_VERSION;
}

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
