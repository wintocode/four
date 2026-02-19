# Four

A 4-operator FM synthesizer for the Expert Sleepers Disting NT.

Inspired by the RYK Algo module and Yamaha DX9/TX81Z architecture.

## Quick Start

1. **Install**: Copy `four.o` to your Disting NT's SD card (in `/plugins/`)
2. **Load**: Turn on Disting NT, navigate to Algorithms, select "Four"
3. **Play**: Connect a MIDI keyboard or V/OCT source to Bus A
4. **Tweak**:
   - Pick an algorithm (try 2, 5, or 8 to start)
   - Adjust operator levels to shape timbre
   - Add feedback on higher-numbered operators for grit

## Key Concepts

- **Algorithms** (11 available): How the 4 operators connect to each other
- **Operators**: Oscillators that can modulate each other or output sound
- **Wave Warp**: Morphs sine → triangle → sawtooth → pulse
- **Wave Fold**: Adds harmonics by folding back waveform peaks

## MIDI

Four responds to these MIDI messages on the selected channel:

| Message | Function |
|---------|----------|
| Note On | Sets base frequency, opens gate (overrides V/OCT) |
| Note Off | Closes gate |
| Pitch Bend | ±2 semitones |

All other parameter control is via CV inputs. Four does not respond to MIDI CC.

## Algorithms

Four uses operator routing where higher-numbered operators modulate lower-numbered ones.

| # | Routing | When to use |
|---|----------|-------------|
| 1 | 4→3→2→1 | Complex evolving textures. Deep modulation chain. |
| 2 | (3+4)→2→1 | Parallel modulators create metallic/clangy tones. |
| 3 | 4→2→1, 3→1 | Two tone layers with shared carrier. Rich bells. |
| 4 | 4→3→1, 2→1 | Dual modulation paths hitting one carrier. Punchy. |
| 5 | 4→3, 2→1 | Two independent voices. Good for intervals. |
| 6 | 4→(1,2,3) | One modulator affects all carriers. Detuned pads. |
| 7 | 4→3, 2, 1 | Mixed FM/additive. Cleaner with feedback on Op3. |
| 8 | 1, 2, 3, 4 | Fully additive. No FM, just mixing. Starting point. |
| 9 | 4→3→(1,2) | Parallel carriers with shared modulator. |
| 10 | (3+4)→(1,2) | Dual modulators into dual carriers. |
| 11 | (2+3+4)→1 | Three modulators ganging on one carrier. |

## Parameter Design

Four separates control into two complementary paradigms:

**Non-CV parameters** (knob) set a base value via the UI. These use a 0-127 range (except Fine Tune and Op Fine, which use ±100 cents for musical precision). These are the values you dial in and save with presets.

**CV parameters** provide full-resolution modulation at audio rate. When a CV is connected, it adds to or offsets the base parameter value, scaled by a depth control. This gives you continuous float precision (32-bit) from your modular environment.

Some parameters have *both* a non-CV version and a CV version:

| Non-CV Parameter | CV Equivalent | Why Both? |
|---|---|---|
| Op Feedback (0-127) | Feedback CV + Depth | Feedback is a key timbral parameter — CV allows expressive, continuous sweeps that 128 MIDI steps can't capture |
| Op Fixed Hz (1-9999) | Fixed Hz CV + Depth | Smooth frequency glides and vibrato in fixed mode need audio-rate control |
| Op Level (0-127) | Level CV + Depth | Already existed — amplitude modulation is fundamental |
| Op Warp (0-127) | Warp CV + Depth | Already existed — morphing waveforms smoothly |
| Op Fold (0-127) | Fold CV + Depth | Already existed — sweeping fold amount for movement |

The non-CV value acts as a **base/offset**, and the CV adds on top of it. Set Feedback to 30 via the UI, then use an envelope on Feedback CV to sweep it dynamically during a note.

**CV Depth parameters** themselves also accept CV modulation ("CV-over-CV-depth"). This lets you modulate *how much* a CV affects its target — for example, using an LFO to slowly open up the amount of amplitude modulation on an operator.

## CV Inputs (53 total, user-assignable)

**Global:**
| CV | Function |
|----|----------|
| V/OCT | 1V/octave pitch control (0V = C4) |
| XM | Cross-modulation depth (scales algorithm connections) |
| FM | Frequency modulation for all operators (±1000Hz) |
| Sync | Hard sync — resets all phases on rising edge |
| Global VCA | Master output level |

**Per-operator (×4 each):**
| CV | Depth Param | Depth CV | Function |
|----|-------------|----------|----------|
| Level CV | Level CV Depth | Level Depth CV | Amplitude modulation |
| PM CV | PM CV Depth | PM Depth CV | Phase modulation |
| Warp CV | Warp CV Depth | Warp Depth CV | Wave warp amount |
| Fold CV | Fold CV Depth | Fold Depth CV | Wave fold amount |
| Feedback CV | Feedback CV Depth | Feedback Depth CV | Self-feedback amount |
| Fixed Hz CV | Fixed Hz CV Depth | Fixed Hz Depth CV | Frequency offset (fixed mode) |

## Wave Shaping

Every operator has wave shaping *before* any modulation or output.

**Wave Warp:** Morphs the base sine through other waveforms.
- 0: Pure sine
- ~42: Triangle
- ~85: Sawtooth
- 127: Pulse/square

Use it to add harmonics before FM hits. More harmonics = richer modulation results.

**Wave Fold:** Folds the waveform back when it exceeds ±1.
- **Symmetric:** Both positive and negative peaks fold
- **Asymmetric:** Only positive peaks fold, negative clips
- **Soft Clip:** Gentle saturation

Small amounts (10-40) add sparkle. High amounts (90+) create distortion.

## About Four

Four is a 4-operator FM synthesizer for Disting NT, created because I wanted the RYK Algo experience in Eurorack format without buying another module.

**Design choices:**
- Mono output only — keeps it focused
- Wave warp and fold on every operator — more timbral range than pure FM
- No built-in envelope — use your own ADSR/VCA
- No MIDI CC — all modulation via CV for full-resolution control
- Every CV parameter also accepts CV modulation of its depth

**Name:** "Four" for the four operators. Simple as that.

## Documentation

- **[DESIGN.md](docs/DESIGN.md)** — Technical design, signal flow, and architecture
- **[NT Gallery](https://nt-gallery.nosuch.dev)** — Download latest release

## Building

Requires the ARM toolchain:
```bash
brew install --cask gcc-arm-embedded
```

Build:
```bash
make
```

Output: `plugins/four.o`

## Testing

Desktop tests:
```bash
cd tests && make run
```

## Versioning

This project uses [Semantic Versioning](https://semver.org).

- **MAJOR**: Breaking changes to preset compatibility/API
- **MINOR**: New features
- **PATCH**: Bug fixes

Pre-release identifiers indicate stability:
- `-beta.N`: Testing phase, may have bugs
- `-rc.N`: Release candidate, mostly stable
- No suffix: Stable release

Example: `1.1.0-beta.2` → `1.1.0-rc.1` → `1.1.0`

## Author

wintocode
