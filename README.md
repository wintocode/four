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

## MIDI CC Reference

| CC | Parameter | CC | Parameter |
|----|-----------|----|----------|
| 14 | Algorithm | 15 | XM |
| 16 | Fine Tune | 17 | Oversampling |
| 18 | PolyBLEP | 19 | MIDI Channel* |
| 20 | Global VCA | 21-23 | Op1: Freq Mode, Coarse, Fixed Hz |
| 24-26 | Op1: Fine, Level, Feedback | 27-29 | Op1: Warp, Fold, Fold Type |
| 30-38 | Op2 (all params) | 39-47 | Op3 (all params) |
| 48-56 | Op4 (all params) | 57-60 | Op1-4 Level CV Depth |
| 61-64 | Op1-4 PM CV Depth | 65-68 | Op1-4 Warp CV Depth |
| 69-72 | Op1-4 Fold CV Depth | | |

*CC 19 sets channel, but messages only respond on the configured channel

Additional MIDI:
- **Pitch Bend**: ±2 semitones
- **Note On/Off**: Sets base frequency (overrides V/OCT when gate is on)

### Using MIDI CCs

**Value scaling:** CCs use 0-127, scaled to each parameter's range:
- Percentages (0-100) → 0-127 maps linearly
- Cents (-100 to +100) → 64 is center, lower is flat, higher is sharp
- Enums (Algorithms, types) → Each value is a consecutive CC number

**Performance tips:**
- **XM (CC 15)**: Great for live modulation. Start subtle (0-40) for evolving pads, crank it (80-127) for metallic chaos.
- **Op Levels (CCs 25, 34, 43, 52)**: The primary way to shape timbre.
- **Feedback (CCs 26, 35, 44, 53)**: Small amounts (10-30) add growl; high amounts create noise.
- **Warp (CCs 27, 36, 45, 54)**: 0-42 = sine territory, 43-85 = saw/triangle, 86+ = pulse harmonics.
- **Fold (CCs 28, 37, 46, 55)**: 0-30 adds sparkle, 31-70 adds aggression, 71+ creates distortion.

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

## CV Inputs (20 total, user-assignable)

**Global:**
| CV | Function |
|----|----------|
| V/OCT | 1V/octave pitch control (0V = C4) |
| XM | Cross-modulation depth (scales algorithm connections) |
| FM | Frequency modulation for all operators (±1000Hz) |
| Sync | Hard sync — resets all phases on rising edge |
| Global VCA | Master output level |

**Per-operator (4 CV each):**
| CV | Function |
|----|----------|
| Level | Amplitude modulation of that operator |
| PM | Phase modulation into that operator |
| Warp | Wave warp amount |
| Fold | Wave fold amount |

## Wave Shaping

Every operator has wave shaping *before* any modulation or output.

**Wave Warp:** Morphs the base sine through other waveforms.
- 0%: Pure sine
- 33%: Triangle
- 67%: Sawtooth
- 100%: Pulse/square

Use it to add harmonics before FM hits. More harmonics = richer modulation results.

**Wave Fold:** Folds the waveform back when it exceeds ±1.
- **Symmetric:** Both positive and negative peaks fold
- **Asymmetric:** Only positive peaks fold, negative clips
- **Soft Clip:** Gentle saturation

Small amounts (10-30%) add sparkle. High amounts (70%+) create distortion.

## About Four

Four is a 4-operator FM synthesizer for Disting NT, created because I wanted the RYK Algo experience in Eurorack format without buying another module.

**Design choices:**
- Mono output only — keeps it focused
- Wave warp and fold on every operator — more timbral range than pure FM
- No built-in envelope — use your own ADSR/VCA
- MIDI CCs for every parameter — playable from a keyboard

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
