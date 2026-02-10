# Four FM Synthesizer — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build a 4-operator FM/PM synthesizer plugin for the Expert Sleepers Disting NT.

**Architecture:** Single-compilation-unit plugin (`src/four.cpp`) that includes a pure-DSP header (`src/dsp.h`) with no API dependencies. DSP functions are testable on desktop via a separate test build. The plugin compiles to a `.o` file loaded by the Disting NT firmware on ARM Cortex-M7.

**Tech Stack:** C++11, ARM cross-compiler (`arm-none-eabi-c++`), Disting NT API v1.14.0, clang++ for desktop tests.

**Design doc:** `docs/DESIGN.md`

---

## File Structure

```
Four/
├── distingNT_API/              # Git submodule (Disting NT API)
│   └── include/distingnt/api.h
├── src/
│   ├── four.cpp                # Main plugin (entry, factory, construct, step, MIDI, params)
│   └── dsp.h                   # Pure DSP functions (no API dependency, testable on desktop)
├── tests/
│   ├── test_dsp.cpp            # Desktop tests for DSP functions
│   └── Makefile                # Desktop test build (clang++)
├── plugins/                    # Build output (.o files for Disting NT)
├── Makefile                    # ARM cross-compilation
└── docs/
    ├── DESIGN.md
    └── plans/
```

## Algorithm Definitions

Eight DX9-style algorithms. Operators numbered 1-4. Higher-numbered operators modulate lower. Processing order: 4, 3, 2, 1 (compute modulators before their targets).

Using indices 0-3 for ops 1-4. `mod[src][dst]` = source modulates destination.

Standard TX81Z/DX9 4-operator algorithms:

| #  | Routing                   | Modulations                      | Carriers   |
|----|---------------------------|----------------------------------|------------|
| 1  | 4→3→2→1                  | 4→3, 3→2, 2→1                   | {1}        |
| 2  | (3+4)→2→1                | 3→2, 4→2, 2→1                   | {1}        |
| 3  | (4→2→1) + (3→1)          | 4→2, 2→1, 3→1                   | {1}        |
| 4  | (4→3→1) + (2→1)          | 4→3, 3→1, 2→1                   | {1}        |
| 5  | (4→3) + (2→1)            | 4→3, 2→1                        | {1, 3}     |
| 6  | 4→(1,2,3)                | 4→1, 4→2, 4→3                   | {1, 2, 3}  |
| 7  | (4→3) + 2 + 1            | 4→3                              | {1, 2, 3}  |
| 8  | 1 + 2 + 3 + 4            | (none)                           | {1,2,3,4}  |

## Parameter Map

62 total parameters. Values are `int16_t` accessed via `self->v[index]`.

**I/O (2):**
| Index | Name           | Min | Max | Default | Unit            |
|-------|----------------|-----|-----|---------|-----------------|
| 0     | Output         | 1   | 28  | 13      | Audio Output    |
| 1     | Output Mode    | 0   | 1   | 0       | Output Mode     |

**Global (7):**
| Index | Name           | Min | Max | Default | Unit    | Notes                    |
|-------|----------------|-----|-----|---------|---------|--------------------------|
| 2     | Algorithm      | 0   | 7   | 0       | Enum    | strings: "1".."8"        |
| 3     | XM             | 0   | 100 | 0       | Percent |                          |
| 4     | Fine Tune      | -100| 100 | 0       | Cents   |                          |
| 5     | Oversampling   | 0   | 1   | 0       | Enum    | "Off","2x"               |
| 6     | PolyBLEP       | 0   | 1   | 0       | Enum    | "Off","On"               |
| 7     | MIDI Channel   | 1   | 16  | 1       | None    |                          |
| 8     | Global VCA     | 0   | 100 | 100     | Percent |                          |

**Per-Operator ×4 (32 total, indices 9-40):**
Each operator block is 8 parameters. Op N starts at index `9 + (N-1)*8`.

| Offset | Name       | Min  | Max   | Default | Unit    | Notes                              |
|--------|------------|------|-------|---------|---------|------------------------------------|
| +0     | Freq Mode  | 0    | 1     | 0       | Enum    | "Ratio","Fixed"                    |
| +1     | Coarse     | 1    | 32    | 1       | None    | Dynamic: 1-9999 in Fixed mode      |
| +2     | Fine       | -100 | 100   | 0       | Cents   |                                    |
| +3     | Level      | 0    | 100   | 100     | Percent |                                    |
| +4     | Feedback   | 0    | 100   | 0       | Percent |                                    |
| +5     | Wave Warp  | 0    | 100   | 0       | Percent |                                    |
| +6     | Wave Fold  | 0    | 100   | 0       | Percent |                                    |
| +7     | Fold Type  | 0    | 2     | 0       | Enum    | "Symmetric","Asymmetric","SoftClip"|

**CV Inputs (21 total, indices 41-61):**
All are bus selectors using `NT_PARAMETER_CV_INPUT` macro, min=0, max=28, default=0 (0 = none).

| Index | Name          |
|-------|---------------|
| 41    | V/OCT CV      |
| 42    | XM CV         |
| 43    | FM CV         |
| 44    | Sync CV       |
| 45    | Global VCA CV |
| 46-49 | Op 1-4 AM CV  |
| 50-53 | Op 1-4 PM CV  |
| 54-57 | Op 1-4 Warp CV|
| 58-61 | Op 1-4 Fold CV|

---

## Task 1: Clone API & Install Toolchain

**Files:**
- Create: `.gitmodules`

**Step 1: Add Disting NT API as git submodule**

```bash
cd /Volumes/CODE/Four
git submodule add https://github.com/expertsleepersltd/distingNT_API.git distingNT_API
```

**Step 2: Install ARM cross-compiler**

```bash
brew install --cask gcc-arm-embedded
```

If that fails, try:
```bash
brew install arm-none-eabi-gcc
```

**Step 3: Verify toolchain**

```bash
arm-none-eabi-c++ --version
```

Expected: Version info for ARM GCC.

**Step 4: Verify API header exists**

```bash
ls distingNT_API/include/distingnt/api.h
```

Expected: File exists.

**Step 5: Commit**

```bash
git add .gitmodules distingNT_API
git commit -m "chore: add Disting NT API as git submodule"
```

---

## Task 2: Create Build System

**Files:**
- Create: `Makefile`
- Create: `tests/Makefile`

**Step 1: Create ARM Makefile**

Create `Makefile`:

```makefile
NT_API_PATH := distingNT_API
INCLUDE_PATH := $(NT_API_PATH)/include
SRC := src/four.cpp
OUTPUT := plugins/four.o

CC := arm-none-eabi-c++
CFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
          -mthumb -fno-rtti -fno-exceptions -Os -fPIC -Wall \
          -I$(INCLUDE_PATH) -Isrc

all: $(OUTPUT)

$(OUTPUT): $(SRC) src/dsp.h
	mkdir -p plugins
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OUTPUT)

.PHONY: all clean
```

**Step 2: Create test Makefile**

Create `tests/Makefile`:

```makefile
CC := c++
CFLAGS := -std=c++11 -Wall -Wextra -g -I../src -fsanitize=address,undefined
SRC := test_dsp.cpp
OUTPUT := test_dsp

all: $(OUTPUT)

$(OUTPUT): $(SRC) ../src/dsp.h
	$(CC) $(CFLAGS) -o $@ $< -lm

run: $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)

.PHONY: all run clean
```

**Step 3: Commit**

```bash
git add Makefile tests/Makefile
git commit -m "chore: add ARM and desktop test build systems"
```

---

## Task 3: Skeleton Plugin

**Files:**
- Create: `src/four.cpp`
- Create: `src/dsp.h`

**Step 1: Create empty DSP header**

Create `src/dsp.h`:

```cpp
#ifndef FOUR_DSP_H
#define FOUR_DSP_H

// Pure DSP functions for Four FM synthesizer.
// No Disting NT API dependencies — testable on desktop.

#include <math.h>

namespace four {

} // namespace four

#endif // FOUR_DSP_H
```

**Step 2: Create skeleton plugin**

Create `src/four.cpp`:

```cpp
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
```

**Step 3: Build for ARM**

```bash
make clean && make
```

Expected: `plugins/four.o` is created with no errors.

**Step 4: Commit**

```bash
git add src/four.cpp src/dsp.h
git commit -m "feat: skeleton plugin that compiles for ARM"
```

---

## Task 4: Desktop Test Harness

**Files:**
- Create: `tests/test_dsp.cpp`

**Step 1: Create test runner**

Create `tests/test_dsp.cpp`:

```cpp
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
```

**Step 2: Build and run tests**

```bash
cd tests && make run
```

Expected:
```
Four DSP tests:
  placeholder ... PASS

1/1 tests passed.
```

**Step 3: Commit**

```bash
git add tests/test_dsp.cpp
git commit -m "feat: desktop test harness for DSP functions"
```

---

## Task 5: Parameter Definitions

**Files:**
- Modify: `src/four.cpp`

This is the largest single task — define all 62 parameters, their enums, and page layout.

**Step 1: Write the parameter enum**

Replace the existing enum and parameters in `src/four.cpp`:

```cpp
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
// Helper: first AM CV param index for operator N (0-based)
static inline int opAMCV( int op ) { return kParamOp1AMCV + op; }
static inline int opPMCV( int op ) { return kParamOp1PMCV + op; }
static inline int opWarpCV( int op ) { return kParamOp1WarpCV + op; }
static inline int opFoldCV( int op ) { return kParamOp1FoldCV + op; }
```

**Step 2: Write enum strings**

```cpp
static const char* algorithmStrings[] = { "1","2","3","4","5","6","7","8", NULL };
static const char* offOnStrings[]     = { "Off","On", NULL };
static const char* off2xStrings[]     = { "Off","2x", NULL };
static const char* freqModeStrings[]  = { "Ratio","Fixed", NULL };
static const char* foldTypeStrings[]  = { "Symmetric","Asymmetric","Soft Clip", NULL };
```

**Step 3: Write the parameter definitions array**

```cpp
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
    { "Fine Tune",  -100, 100,   0,   kNT_unitCents,  0, NULL },
    { "Oversampling", 0,    1,   0,   kNT_unitEnum,    0, off2xStrings },
    { "PolyBLEP",     0,    1,   0,   kNT_unitEnum,    0, offOnStrings },
    { "MIDI Channel", 1,   16,   1,   kNT_unitNone,   0, NULL },
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
```

**Step 4: Write parameter pages**

```cpp
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
    { "I/O",          ARRAY_SIZE(pageIO),       0, pageIO },
    { "Global",       ARRAY_SIZE(pageGlobal),   0, pageGlobal },
    { "MIDI",         ARRAY_SIZE(pageMIDI),     0, pageMIDI },
    { "Operator 1",   ARRAY_SIZE(pageOp1),      1, pageOp1 },
    { "Operator 2",   ARRAY_SIZE(pageOp2),      1, pageOp2 },
    { "Operator 3",   ARRAY_SIZE(pageOp3),      1, pageOp3 },
    { "Operator 4",   ARRAY_SIZE(pageOp4),      1, pageOp4 },
    { "CV Global",    ARRAY_SIZE(pageCVGlobal),  2, pageCVGlobal },
    { "CV Op1",       ARRAY_SIZE(pageCVOp1),     2, pageCVOp1 },
    { "CV Op2",       ARRAY_SIZE(pageCVOp2),     2, pageCVOp2 },
    { "CV Op3",       ARRAY_SIZE(pageCVOp3),     2, pageCVOp3 },
    { "CV Op4",       ARRAY_SIZE(pageCVOp4),     2, pageCVOp4 },
};

static const _NT_parameterPages parameterPages = {
    ARRAY_SIZE(pages), pages
};
```

**Step 5: Update construct to assign pages**

In `construct()`, add after `alg->parameters = parameters;`:

```cpp
alg->parameterPages = &parameterPages;
```

**Step 6: Update kNumParams and build**

Verify `kNumParams` in the enum equals `ARRAY_SIZE(parameters)`. Build:

```bash
make clean && make
```

Expected: Compiles with no errors.

**Step 7: Commit**

```bash
git add src/four.cpp
git commit -m "feat: define all 62 parameters with pages"
```

---

## Task 6: Algorithm State & parameterChanged

**Files:**
- Modify: `src/four.cpp`

**Step 1: Expand the algorithm struct**

Replace `_fourAlgorithm` with the full state:

```cpp
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
```

**Step 2: Implement parameterChanged**

```cpp
static void parameterChanged( _NT_algorithm* self, uint32_t parameter )
{
    _fourAlgorithm* p = (_fourAlgorithm*)self;

    // Per-operator parameters
    for ( int op = 0; op < 4; ++op )
    {
        int base = kParamOp1FreqMode + op * 8;
        if ( parameter >= (uint32_t)base && parameter < (uint32_t)(base + 8) )
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
```

**Step 3: Wire parameterChanged into factory**

Update the factory definition:

```cpp
.parameterChanged = parameterChanged,
```

**Step 4: Build**

```bash
make clean && make
```

Expected: Compiles with no errors.

**Step 5: Commit**

```bash
git add src/four.cpp
git commit -m "feat: algorithm state struct and parameterChanged handler"
```

---

## Task 7: Oscillator — Phase Accumulator + Sine

**Files:**
- Modify: `src/dsp.h`
- Modify: `tests/test_dsp.cpp`

**Step 1: Write the failing test**

Add to `tests/test_dsp.cpp`, before `main()`:

```cpp
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
```

Add calls in `main()`:

```cpp
run_oscillator_sine_zero_phase();
run_oscillator_sine_quarter();
run_oscillator_sine_half();
run_phase_advance();
run_phase_advance_wraps();
```

**Step 2: Run tests — verify they fail**

```bash
cd tests && make run
```

Expected: Compilation error — functions not defined.

**Step 3: Implement in dsp.h**

Add inside `namespace four`:

```cpp
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
```

**Step 4: Run tests — verify they pass**

```bash
cd tests && make run
```

Expected: All tests pass.

**Step 5: Commit**

```bash
git add src/dsp.h tests/test_dsp.cpp
git commit -m "feat: phase accumulator and sine oscillator"
```

---

## Task 8: Frequency Calculation

**Files:**
- Modify: `src/dsp.h`
- Modify: `tests/test_dsp.cpp`

**Step 1: Write the failing tests**

```cpp
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
```

**Step 2: Run tests — verify they fail**

```bash
cd tests && make run
```

Expected: Compilation error.

**Step 3: Implement in dsp.h**

```cpp
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
```

**Step 4: Run tests — verify they pass**

```bash
cd tests && make run
```

**Step 5: Commit**

```bash
git add src/dsp.h tests/test_dsp.cpp
git commit -m "feat: frequency calculation (ratio, fixed, V/OCT, MIDI)"
```

---

## Task 9: Wave Warp

**Files:**
- Modify: `src/dsp.h`
- Modify: `tests/test_dsp.cpp`

Wave warp morphs between sine → triangle → sawtooth → pulse as warp goes 0 → 1.

**Step 1: Write the failing tests**

```cpp
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
```

**Step 2: Run tests — verify they fail**

**Step 3: Implement in dsp.h**

The approach: generate each waveform from phase, then crossfade based on warp amount.

```cpp
// Raw waveform generators from normalized phase [0, 1)
inline float waveform_triangle( float phase )
{
    // Triangle: rises 0→1 in [0, 0.25], falls 1→-1 in [0.25, 0.75], rises -1→0 in [0.75, 1]
    if ( phase < 0.25f )
        return phase * 4.0f;
    else if ( phase < 0.75f )
        return 2.0f - phase * 4.0f;
    else
        return phase * 4.0f - 4.0f;
}

inline float waveform_saw( float phase )
{
    // Sawtooth: rises from -1 to +1 over one cycle
    return 2.0f * phase - 1.0f;
}

inline float waveform_pulse( float phase )
{
    // Pulse/square: +1 for first half, -1 for second half
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
        float t = warp * 3.0f;  // 0-1 within this segment
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
```

**Step 4: Run tests — verify they pass**

```bash
cd tests && make run
```

**Step 5: Commit**

```bash
git add src/dsp.h tests/test_dsp.cpp
git commit -m "feat: wave warp (sine → triangle → saw → pulse)"
```

---

## Task 10: Wave Fold

**Files:**
- Modify: `src/dsp.h`
- Modify: `tests/test_dsp.cpp`

Three fold types: Symmetric, Asymmetric, Soft Clip. Fold amount drives the signal before the folding function.

**Step 1: Write the failing tests**

```cpp
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
```

**Step 2: Run tests — verify they fail**

**Step 3: Implement in dsp.h**

```cpp
// Soft clipping function (tanh approximation, fast)
inline float soft_clip( float x )
{
    // Pade approximant of tanh, accurate for |x| < 3
    if ( x < -3.0f ) return -1.0f;
    if ( x >  3.0f ) return  1.0f;
    float x2 = x * x;
    return x * ( 27.0f + x2 ) / ( 27.0f + 9.0f * x2 );
}

// Symmetric fold: sin-based fold that wraps signal back within [-1, 1]
inline float fold_symmetric( float x )
{
    // Use sin to create smooth folding
    return sinf( x * 1.5707963f );  // sin(x * π/2), folds at ±1
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
```

**Step 4: Run tests — verify they pass**

**Step 5: Commit**

```bash
git add src/dsp.h tests/test_dsp.cpp
git commit -m "feat: wave fold (symmetric, asymmetric, soft clip)"
```

---

## Task 11: Self-Feedback

**Files:**
- Modify: `src/dsp.h`
- Modify: `tests/test_dsp.cpp`

Feedback feeds the operator's previous output back into its own phase input, soft-clipped to prevent runaway.

**Step 1: Write the failing tests**

```cpp
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
```

**Step 2: Run tests — verify they fail**

**Step 3: Implement in dsp.h**

```cpp
// Calculate feedback contribution from previous output
// prev_output: previous sample output, amount: 0.0-1.0
// Returns phase modulation amount (bounded)
inline float calc_feedback( float prev_output, float amount )
{
    return soft_clip( prev_output * amount );
}
```

**Step 4: Run tests — verify they pass**

**Step 5: Commit**

```bash
git add src/dsp.h tests/test_dsp.cpp
git commit -m "feat: self-feedback with soft clipping"
```

---

## Task 12: Algorithm Routing

**Files:**
- Modify: `src/dsp.h`
- Modify: `tests/test_dsp.cpp`

Data-driven algorithm definitions: modulation matrix + carrier flags.

**Step 1: Write the failing tests**

```cpp
TEST(algorithm_1_serial_chain)
{
    // Algo 1: 4→3→2→1, carrier: 1
    const four::Algorithm& a = four::algorithms[0];
    // Op4 (idx 3) modulates op3 (idx 2)
    ASSERT( a.mod[3][2] );
    // Op3 modulates op2
    ASSERT( a.mod[2][1] );
    // Op2 modulates op1
    ASSERT( a.mod[1][0] );
    // Only op1 is carrier
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
```

**Step 2: Run tests — verify they fail**

**Step 3: Implement in dsp.h**

```cpp
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
    // Op4 modulates op2 (skipping op3), op2 and op3 both mod op1
    { { {0,0,0,0}, {1,0,0,0}, {1,0,0,0}, {0,1,0,0} },
      {true, false, false, false} },

    // Algo 4: (4→3→1) + (2→1), carriers: {1}
    // Op4 modulates op3, op3 and op2 both mod op1
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
// target: operator index (0-3)
// opOut: raw outputs of all 4 operators (pre-level)
// level: operator levels [0-1]
// xm: cross-modulation master depth [0-1]
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
```

**Important note on the modulation matrix layout:**

The matrix is `mod[src][dst]`. Each row is a source operator, each column is a destination.
For Algo 1 (4→3→2→1):
- Row 0 (op1): `{0,0,0,0}` — op1 modulates nothing
- Row 1 (op2): `{1,0,0,0}` — op2 modulates op1
- Row 2 (op3): `{0,1,0,0}` — op3 modulates op2
- Row 3 (op4): `{0,0,1,0}` — op4 modulates op3

**Step 4: Run tests — verify they pass**

```bash
cd tests && make run
```

**Step 5: Commit**

```bash
git add src/dsp.h tests/test_dsp.cpp
git commit -m "feat: 8 algorithm definitions with routing and carrier summing"
```

---

## Task 13: Complete step() Function

**Files:**
- Modify: `src/four.cpp`

This wires everything together: oscillators + algorithm routing + XM + wave shaping + VCA.

**Step 1: Write the full step() function**

Replace the existing `step()`:

```cpp
static void step(
    _NT_algorithm* self,
    float* busFrames,
    int numFramesBy4 )
{
    _fourAlgorithm* p = (_fourAlgorithm*)self;
    int numFrames = numFramesBy4 * 4;

    float* out = busFrames + ( p->v[kParamOutput] - 1 ) * numFrames;
    bool replace = p->v[kParamOutputMode];

    float sampleRate = (float)NT_globals.sampleRate;
    const four::Algorithm& algo = four::algorithms[p->algorithm];

    // Read CV buses (0 = not connected)
    const float* cvVOct     = p->v[kParamVOctCV]     ? busFrames + (p->v[kParamVOctCV] - 1) * numFrames     : NULL;
    const float* cvXM       = p->v[kParamXMCV]       ? busFrames + (p->v[kParamXMCV] - 1) * numFrames       : NULL;
    const float* cvFM       = p->v[kParamFMCV]       ? busFrames + (p->v[kParamFMCV] - 1) * numFrames       : NULL;
    const float* cvSync     = p->v[kParamSyncCV]     ? busFrames + (p->v[kParamSyncCV] - 1) * numFrames     : NULL;
    const float* cvGlobalVCA= p->v[kParamGlobalVCACV]? busFrames + (p->v[kParamGlobalVCACV] - 1) * numFrames: NULL;

    const float* cvAM[4];
    const float* cvPM[4];
    const float* cvWarp[4];
    const float* cvFold[4];
    for ( int op = 0; op < 4; ++op )
    {
        int16_t bus;
        bus = p->v[opAMCV(op)];   cvAM[op]   = bus ? busFrames + (bus-1)*numFrames : NULL;
        bus = p->v[opPMCV(op)];   cvPM[op]   = bus ? busFrames + (bus-1)*numFrames : NULL;
        bus = p->v[opWarpCV(op)]; cvWarp[op] = bus ? busFrames + (bus-1)*numFrames : NULL;
        bus = p->v[opFoldCV(op)]; cvFold[op] = bus ? busFrames + (bus-1)*numFrames : NULL;
    }

    // Sync state (edge detection)
    float prevSync = p->dsBuffer[1];  // Reuse dsBuffer[1] for sync edge state

    // Pre-compute operator frequencies
    float opFreq[4];
    for ( int op = 0; op < 4; ++op )
    {
        float base = p->baseFrequency * p->pitchBendFactor * p->fineTune;
        if ( p->opFreqMode[op] == 0 )  // Ratio
            opFreq[op] = four::calc_frequency_ratio( base, p->opCoarse[op], p->opFine[op] );
        else  // Fixed
            opFreq[op] = four::calc_frequency_fixed( p->opCoarse[op], p->opFine[op] );
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
                float fm = cvFM ? cvFM[i] * 1000.0f : 0.0f;  // FM CV: ±5V → ±5000Hz
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

        // --- Process operators (4 → 3 → 2 → 1) ---
        float opOut[4];

        for ( int op = 3; op >= 0; --op )
        {
            // Phase increment
            float inc = opFreq[op] / sampleRate;

            // Gather modulation from algorithm routing
            float pm = four::gather_modulation( op, opOut, p->opLevel, xm, algo );

            // Self-feedback
            pm += four::calc_feedback( p->prevOutput[op], p->opFeedback[op] );

            // External PM CV
            if ( cvPM[op] )
                pm += cvPM[op][i];

            // Advance phase
            four::phase_advance( p->phase[op], inc );

            // Compute waveform
            float modPhase = p->phase[op] + pm;
            modPhase -= floorf( modPhase );  // Wrap to [0, 1)

            // Warp amount with CV
            float warp = p->opWarp[op];
            if ( cvWarp[op] )
                warp = fminf( 1.0f, fmaxf( 0.0f, warp + cvWarp[op][i] * 0.2f ) );

            float sample;
            if ( warp > 0.0f )
                sample = four::wave_warp( modPhase, warp );
            else
                sample = four::oscillator_sine( modPhase );

            // Fold amount with CV
            float fold = p->opFold[op];
            if ( cvFold[op] )
                fold = fminf( 1.0f, fmaxf( 0.0f, fold + cvFold[op][i] * 0.2f ) );

            if ( fold > 0.0f )
                sample = four::wave_fold( sample, fold, p->opFoldType[op] );

            // AM CV
            if ( cvAM[op] )
                sample *= fmaxf( 0.0f, 1.0f + cvAM[op][i] );

            opOut[op] = sample;
            p->prevOutput[op] = sample;
        }

        // --- Sum carriers ---
        float mix = four::sum_carriers( opOut, p->opLevel, algo );

        // Global VCA with CV
        float vca = p->globalVCA;
        if ( cvGlobalVCA )
            vca *= fmaxf( 0.0f, cvGlobalVCA[i] * 0.2f );

        mix *= vca;

        // Output
        if ( replace )
            out[i] = mix;
        else
            out[i] += mix;
    }

    p->dsBuffer[1] = prevSync;  // Store sync state
}
```

**Step 2: Build for ARM**

```bash
make clean && make
```

Expected: Compiles with no errors.

**Step 3: Commit**

```bash
git add src/four.cpp
git commit -m "feat: complete step() — oscillators, routing, XM, wave shaping, VCA"
```

---

## Task 14: MIDI Handling

**Files:**
- Modify: `src/four.cpp`

**Step 1: Implement midiMessage**

```cpp
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
```

**Step 2: Wire midiMessage into factory**

Update factory:

```cpp
.midiMessage = midiMessage,
```

**Step 3: Build**

```bash
make clean && make
```

Expected: Compiles.

**Step 4: Commit**

```bash
git add src/four.cpp
git commit -m "feat: MIDI note on/off and pitch bend"
```

---

## Task 15: Dynamic Frequency Mode Update

**Files:**
- Modify: `src/four.cpp`

When frequency mode changes between Ratio and Fixed, update the Coarse parameter's range dynamically so the hardware UI clamps correctly.

**Step 1: Verify the parameterChanged logic from Task 6 is correct**

The `kOpFreqMode` case in `parameterChanged` already calls `NT_updateParameterDefinition`. Verify it works by cross-referencing with the API. If the parameter definition is non-const (it is — we used `static _NT_parameter parameters[]` without `const`), this should work.

**Step 2: Also update V/OCT → baseFrequency on construct**

In `construct()`, after initializing state, set default base frequency from the default V/OCT assumption (0V = C4):

```cpp
alg->baseFrequency = 261.63f;  // C4
```

This is already done in the constructor — verify it's there.

**Step 3: Build and verify**

```bash
make clean && make
```

**Step 4: Commit** (if any changes were made)

```bash
git add src/four.cpp
git commit -m "fix: verify dynamic parameter update on freq mode change"
```

---

## Task 16: 2× Oversampling

**Files:**
- Modify: `src/dsp.h`
- Modify: `src/four.cpp`
- Modify: `tests/test_dsp.cpp`

**Step 1: Write the failing test**

```cpp
TEST(downsample_2x)
{
    // Simple half-band filter: average of two samples
    // More precisely: a 2-tap FIR [0.5, 0.5] for basic 2x downsample
    float s0 = 0.8f;
    float s1 = 0.6f;
    float result = four::downsample_2x( s0, s1 );
    ASSERT_NEAR( result, 0.7f, 1e-6f );
}
```

**Step 2: Run tests — verify they fail**

**Step 3: Implement in dsp.h**

```cpp
// Simple 2× downsampler (half-band average)
// s0: first sample (even), s1: second sample (odd)
inline float downsample_2x( float s0, float s1 )
{
    return ( s0 + s1 ) * 0.5f;
}
```

**Step 4: Run tests — verify they pass**

**Step 5: Modify step() to support oversampling**

In `step()`, when oversampling is enabled, run the inner loop at 2× rate:

```cpp
// Inside step(), replace the main loop with:
int actualRate = p->oversample ? 2 : 1;
float effectiveSampleRate = sampleRate * (float)actualRate;

for ( int i = 0; i < numFrames; ++i )
{
    float sample = 0.0f;

    for ( int os = 0; os < actualRate; ++os )
    {
        // [... same per-sample DSP code as before, using effectiveSampleRate ...]
        // Store result in `subSample`

        if ( actualRate == 1 )
            sample = subSample;
        else if ( os == 0 )
            p->dsBuffer[0] = subSample;
        else
            sample = four::downsample_2x( p->dsBuffer[0], subSample );
    }

    if ( replace )
        out[i] = sample;
    else
        out[i] += sample;
}
```

The refactoring needed: extract the per-sample DSP into a local lambda or inline section that can be called once (no oversampling) or twice (2× oversampling) per output sample. Use `effectiveSampleRate` instead of `sampleRate` for phase increments.

**Step 6: Build for ARM**

```bash
make clean && make
```

**Step 7: Commit**

```bash
git add src/dsp.h src/four.cpp tests/test_dsp.cpp
git commit -m "feat: 2x oversampling with half-band downsampling"
```

---

## Task 17: PolyBLEP Anti-Aliasing

**Files:**
- Modify: `src/dsp.h`
- Modify: `tests/test_dsp.cpp`

PolyBLEP corrects discontinuities in saw and pulse waveforms to reduce aliasing.

**Step 1: Write the failing tests**

```cpp
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
```

**Step 2: Run tests — verify they fail**

**Step 3: Implement in dsp.h**

```cpp
// PolyBLEP correction for discontinuities
// phase: normalized [0, 1), dt: phase increment per sample
// Returns correction to subtract from waveform at discontinuity points
inline float polyblep( float phase, float dt )
{
    // Near phase = 0 (beginning of cycle)
    if ( phase < dt )
    {
        float t = phase / dt;
        return t + t - t * t - 1.0f;
    }
    // Near phase = 1 (end of cycle)
    else if ( phase > 1.0f - dt )
    {
        float t = ( phase - 1.0f ) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// PolyBLEP-corrected saw
inline float waveform_saw_blep( float phase, float dt )
{
    return waveform_saw( phase ) - polyblep( phase, dt );
}

// PolyBLEP-corrected pulse
inline float waveform_pulse_blep( float phase, float dt )
{
    float p = waveform_pulse( phase );
    p += polyblep( phase, dt );               // Rising edge at 0
    float shifted = phase + 0.5f;
    if ( shifted >= 1.0f ) shifted -= 1.0f;
    p -= polyblep( shifted, dt );             // Falling edge at 0.5
    return p;
}
```

**Step 4: Run tests — verify they pass**

**Step 5: Integrate PolyBLEP into wave_warp**

Add a variant of `wave_warp` that uses PolyBLEP-corrected waveforms when enabled:

```cpp
// Wave warp with optional PolyBLEP (for anti-aliasing saw/pulse)
inline float wave_warp_blep( float phase, float warp, float dt )
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
        float saw = waveform_saw_blep( phase, dt );
        return tri + t * ( saw - tri );
    }
    else
    {
        float t = ( warp - 2.0f / 3.0f ) * 3.0f;
        float saw = waveform_saw_blep( phase, dt );
        float pls = waveform_pulse_blep( phase, dt );
        return saw + t * ( pls - saw );
    }
}
```

**Step 6: Use wave_warp_blep in step() when PolyBLEP is enabled**

In `step()`, replace the warp call:

```cpp
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
```

**Step 7: Build for ARM**

```bash
make clean && make
```

**Step 8: Commit**

```bash
git add src/dsp.h src/four.cpp tests/test_dsp.cpp
git commit -m "feat: PolyBLEP anti-aliasing for warped waveforms"
```

---

## Task 18: Final Build & Test on Hardware

**Step 1: Full clean build**

```bash
make clean && make
```

**Step 2: Run all desktop tests**

```bash
cd tests && make clean && make run
```

Expected: All tests pass.

**Step 3: Verify .o file**

```bash
arm-none-eabi-objdump -t plugins/four.o | grep pluginEntry
```

Expected: `pluginEntry` symbol is present.

**Step 4: Check binary size**

```bash
ls -la plugins/four.o
arm-none-eabi-size plugins/four.o
```

Expected: Reasonable size (under 64KB).

**Step 5: Deploy to Disting NT**

Copy `plugins/four.o` to the Disting NT's SD card `plugins/` directory and test:
1. Plugin loads and appears in algorithm list
2. Parameter pages display correctly
3. Audio output produces sound when V/OCT or MIDI is connected
4. Algorithm selection changes timbre
5. XM knob varies modulation depth
6. Wave warp and fold are audible
7. MIDI note on/off works
8. Oversampling and PolyBLEP options work

**Step 6: Final commit**

```bash
git add -A
git commit -m "feat: Four v1.0 — 4-op FM synthesizer for Disting NT"
```

---

## Summary

| Task | Description                          | Type    |
|------|--------------------------------------|---------|
| 1    | Clone API & install toolchain        | Setup   |
| 2    | Create build system                  | Setup   |
| 3    | Skeleton plugin                      | Code    |
| 4    | Desktop test harness                 | Test    |
| 5    | Parameter definitions (62 params)    | Code    |
| 6    | Algorithm state & parameterChanged   | Code    |
| 7    | Oscillator (phase + sine)            | TDD     |
| 8    | Frequency calculation                | TDD     |
| 9    | Wave warp                            | TDD     |
| 10   | Wave fold (3 types)                  | TDD     |
| 11   | Self-feedback                        | TDD     |
| 12   | Algorithm routing (8 algos)          | TDD     |
| 13   | Complete step() function             | Code    |
| 14   | MIDI handling                        | Code    |
| 15   | Dynamic freq mode update             | Code    |
| 16   | 2× oversampling                      | TDD     |
| 17   | PolyBLEP anti-aliasing               | TDD     |
| 18   | Final build & hardware test          | Deploy  |
