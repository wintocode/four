# Four — Design Document

## Overview

Four is a 4-operator FM/PM synthesizer plugin for the Expert Sleepers Disting NT,
inspired by the RYK Algo module and Yamaha DX9/TX81Z architecture.

Mono output. No stereo, no panning, no chorus.

## Algorithms

8 DX9-style algorithms defining modulator/carrier routing between the 4 operators.
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

- **Algorithm** (1-8)
- **XM** — cross modulation master depth (scales modulator outputs only, not carriers)
- **Fine Tune** (+/- cents)
- **Oversampling**: None / 2× (4 effective options: 48kHz, 48kHz→96kHz, 96kHz, 96kHz→192kHz)
- **PolyBLEP**: On / Off (anti-aliasing for warped waveforms)
- **MIDI channel**
- **Global VCA level**

## CV Inputs (user-assignable to Disting NT buses)

- V/OCT — pitch CV
- XM CV — cross modulation amount
- FM CV — frequency modulation for all oscillators
- Sync — phase reset trigger for all oscillators
- Osc 1-4 AM CV (×4) — per-oscillator amplitude modulation
- Osc 1-4 PM CV (×4) — per-oscillator phase modulation
- Osc 1-4 Warp CV (×4) — per-oscillator wave warp
- Osc 1-4 Fold CV (×4) — per-oscillator wave fold
- Global VCA CV

Total: 20 CV inputs, all independently routable to any bus.

## Audio Output

- Mono out (single bus)

## Signal Flow

```
Per oscillator:
  base_freq = V/OCT (or MIDI note) × ratio (or fixed Hz)
  phase += base_freq + PM_from_algorithm + PM_CV + FM_CV + self_feedback
  waveform = sine(phase)
  waveform = wave_warp(waveform, warp_amount + warp_CV)
  waveform = wave_fold(waveform, fold_amount + fold_CV, fold_type)
  output = waveform × level × AM_CV

Algorithm routing:
  modulator outputs → carrier phase inputs (scaled by XM)
  carrier outputs → summed → × Global VCA → mono out

Sync trigger → reset all phase accumulators to 0
```

## MIDI

- **Note on/off** → sets base frequency (overrides V/OCT when active)
- **Pitch bend** → bends base frequency
- **CC mapping** → plugin-level mapping of MIDI CCs to all parameters
- **MIDI channel** selectable via parameter

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
