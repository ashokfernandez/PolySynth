# PolySynth

Polyphonic synthesizer plugin for macOS and Windows. AU, VST3, and standalone formats. Built with C++ and [iPlug2](https://github.com/iPlug2/iPlug2).

[![CI](https://github.com/ashokfernandez/PolySynth/actions/workflows/ci.yml/badge.svg)](https://github.com/ashokfernandez/PolySynth/actions/workflows/ci.yml)
[![Latest Release](https://img.shields.io/github/v/release/ashokfernandez/PolySynth?display_name=tag)](https://github.com/ashokfernandez/PolySynth/releases/latest)
[![Try it](https://img.shields.io/badge/Try_it-in_your_browser-blue)](https://ashokfernandez.github.io/PolySynth/)

## Try It

Play PolySynth in your browser — no install required: **[ashokfernandez.github.io/PolySynth](https://ashokfernandez.github.io/PolySynth/)**

## Download

Grab the latest installer from [GitHub Releases](https://github.com/ashokfernandez/PolySynth/releases/latest).

- **macOS:** Install the `.pkg`. If Gatekeeper prompts, right-click the installer and choose **Open**.
- **Windows:** Unzip and copy `PolySynth.vst3` into your VST3 folder (usually `C:\Program Files\Common Files\VST3`).

## Features

- 5-voice polyphony with deterministic voice allocation
- Dual oscillators (saw, square, triangle, sine)
- Resonant low-pass filter with multiple models
- ADSR envelopes for amplitude and filter
- LFO with multiple waveforms and routing targets
- FX engine (delay, chorus)
- Preset management (factory + user presets)
- Save and recall your own patches

## Building from Source

### Prerequisites

- CMake 3.14+
- [`just`](https://github.com/casey/just)
- Xcode (macOS) or MSVC (Windows)

### Setup

```bash
git clone --recursive https://github.com/ashokfernandez/PolySynth.git
cd PolySynth
just deps    # fetch iPlug2 + Skia dependencies
just build   # build test targets
just test    # run unit tests
```

### Common Commands

```bash
just                  # list all tasks
just check            # lint + test
just ci-pr            # full PR gate (lint + ASan + TSan + tests + desktop smoke + VRT)
just desktop-rebuild  # rebuild + launch desktop app
just install-local    # build AU + VST3 and install into ~/Library for local DAW testing
just vrt-run          # visual regression checks
```

### Releasing

```bash
just version              # interactive: shows current version, prompts for bump
just version-bump patch   # non-interactive patch bump
just version-show         # print current version
```

Pushing a `v*.*.*` tag triggers CI to build macOS `.pkg` + Windows `.zip` installers and publish them to [GitHub Releases](https://github.com/ashokfernandez/PolySynth/releases).

## Further Reading

| Topic | Link |
|---|---|
| Architecture overview | [`docs/architecture/OVERVIEW.md`](docs/architecture/OVERVIEW.md) |
| Design principles | [`docs/architecture/DESIGN_PRINCIPLES.md`](docs/architecture/DESIGN_PRINCIPLES.md) |
| Testing guide | [`docs/architecture/TESTING_GUIDE.md`](docs/architecture/TESTING_GUIDE.md) |
| All docs | [`docs/README.md`](docs/README.md) |
| Active plans | [`plans/README.md`](plans/README.md) |

## License

MIT — see [`LICENSE`](LICENSE).

## Acknowledgments

- [iPlug2](https://github.com/iPlug2/iPlug2)
- [Catch2](https://github.com/catchorg/Catch2)
- [nlohmann/json](https://github.com/nlohmann/json)
