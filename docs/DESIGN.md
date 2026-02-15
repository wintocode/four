# Four — Design Document

## Overview

Four is a 4-operator FM/PM synthesizer plugin for the Expert Sleepers Disting NT,
inspired by the RYK Algo module and Yamaha DX9/TX81Z architecture.

Mono output. No stereo, no panning, no chorus.

## Algorithms

11 algorithms defining modulator/carrier routing between the 4 operators.
Operators are numbered 1-4. Higher-numbered operators modulate lower-numbered ones.
Carriers output to the mix; modulators feed into other operators' phase inputs.

| # | Routing                  | Carriers     |
|---|--------------------------|--------------|
| 1 | 4→3→2→1                 | 1            |
| 2 | (3+4)→2→1               | 1            |
| 3 | (4→2→1) + (3→1)         | 1            |
| 4 | (4→3→1) + (2→1)         | 1            |
| 5 | (4→3) + (2→1)           | 1, 3         |
| 6 | 4→(1, 2, 3)             | 1, 2, 3      |
| 7 | (4→3) + 2 + 1           | 1, 2, 3      |
| 8 | 1 + 2 + 3 + 4           | all          |

## Per Oscillator (×4)

- **Frequency mode**: Ratio / Fixed
- **Frequency coarse**: harmonic ratio (Ratio mode) or Hz (Fixed mode)
- **Frequency fine**: fine-tune offset
- **Level**: output amount (modulation depth if modulator, volume if carrier)
- **Feedback amount**: continuous, self-feedback only, soft-clipped
- **Wave Warp amount**: morphs sine → triangle → sawtooth → pulse
- **Wave Fold amount**: folds wave peaks inward, adding harmonics
- **Wave Fold type**: Symmetric / Asymmetric / Soft Clip

Any oscillator can have warp and fold applied regardless of carrier/modulator role.

## Global Parameters

- **Algorithm** (1-11)
- **XM** (0-127) — cross modulation master depth (scales modulator outputs only, not carriers)
- **Fine Tune** (±100 cents)
- **Oversampling**: None / 2× (4 effective options: 48kHz, 48kHz→96kHz, 96kHz, 96kHz→192kHz)
- **PolyBLEP**: On / Off (anti-aliasing for warped waveforms)
- **MIDI channel**
- **Global VCA level** (0-127)

## Parameter Resolution Design

### Non-CV Parameters (Discrete, 0-127)

All continuous parameters use a 0-127 integer range for perfect 1:1 MIDI CC mapping.
This gives 128 steps of resolution — enough for static values and coarse control, but not
for smooth audio-rate modulation. These values are set via the UI or MIDI CC and stored
in presets.

**Exception:** Fine Tune and Op Fine use ±100 cents (semitone range) because this has
direct musical meaning that would be lost with an arbitrary 0-127 scale.

### CV Parameters (Continuous, Full Resolution)

For parameters that benefit from smooth, continuous modulation, Four provides dedicated
CV inputs. CV signals are 32-bit float at audio rate — no stepping, no quantization.
Each CV input has an associated **depth** parameter (0-127) that scales how much the
CV signal affects the target.

The non-CV value acts as a **base**, and the CV adds on top:
```
effective_value = base_param + (CV_signal × depth × scale)
```

### CV-over-CV-Depth

Every CV depth parameter also has its own CV bus selector, allowing the depth itself
to be modulated by a CV signal. This enables effects like:
- Using an LFO to slowly open up amplitude modulation
- Using an envelope to control how much a modulator affects pitch
- Voltage-controlled mixing of modulation sources

## CV Inputs (user-assignable to Disting NT buses)

**Global (5):**
- V/OCT — pitch CV
- XM CV — cross modulation amount
- FM CV — frequency modulation for all oscillators
- Sync — phase reset trigger for all oscillators
- Global VCA CV

**Per-operator (18 each, ×4 = 72):**
- Level CV + Level CV Depth + Level Depth CV
- PM CV + PM CV Depth + PM Depth CV
- Warp CV + Warp CV Depth + Warp Depth CV
- Fold CV + Fold CV Depth + Fold Depth CV
- Feedback CV + Feedback CV Depth + Feedback Depth CV
- Fixed Hz CV + Fixed Hz CV Depth + Fixed Hz Depth CV

Total: 77 CV-routing parameters (5 global + 72 per-operator).
Of these, 53 are bus selectors (5 global + 24 per-op main + 24 per-op depth)
and 24 are depth values (6 per operator × 4).

## Audio Output

- Mono out (single bus)

## Signal Flow

```
Per-sample:
  // CV-over-CV-depth modulation
  effective_depth = depth_param + (depth_CV × scale)

  // Frequency
  base_freq = V/OCT (or MIDI note) × ratio (or fixed Hz + fixedHz_CV × fixedHz_depth)

  // Operator processing
  phase += base_freq + PM_from_algorithm + PM_CV × pm_depth + FM_CV + self_feedback_CV
  waveform = sine(phase)
  waveform = wave_warp(waveform, warp_amount + warp_CV × warp_depth)
  waveform = wave_fold(waveform, fold_amount + fold_CV × fold_depth, fold_type)
  effective_level = clamp(level + level_CV × level_depth × scale, 0, 1)
  output = waveform  // scaled by effective_level during routing

Algorithm routing:
  modulator outputs → carrier phase inputs (scaled by XM + XM_CV)
  carrier outputs → summed → × Global VCA → mono out

Sync trigger → reset all phase accumulators to 0
```

## MIDI

- **Note on/off** → sets base frequency (overrides V/OCT when active)
- **Pitch bend** → bends base frequency
- **CC 14-80** → 67 value parameters mapped (all non-bus-selector params)
- **MIDI channel** selectable via parameter

CV bus selectors are not controllable via MIDI CC; they are static routing
set via the Disting NT UI.

## Anti-Aliasing Strategy

- **Oversampling** (selectable): internal 2× processing with downsample filter
- **PolyBLEP** (selectable): polynomial correction on warp-generated discontinuities
- Both are independent and complementary

## Not Included (deliberate)

- Stereo output / panning
- Chorus effect
- Global detune spread
- Range/octave control (V/OCT handles this)
- Built-in envelope (use external CV)
