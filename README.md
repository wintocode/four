# Four

A 4-operator FM/PM synthesizer plugin for the Expert Sleepers Disting NT.

## Features

- 4 operators with configurable algorithms (DX9-style)
- Per-operator feedback, wave warp, and wave fold (3 types)
- Frequency ratio control (coarse + fine), plus Fixed mode
- MIDI CC mapping for all parameters
- Optional 2× oversampling and PolyBLEP anti-aliasing
- Mono output only

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

## Documentation

See [DESIGN.md](docs/DESIGN.md) for full design details.

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
