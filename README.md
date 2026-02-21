# PolySynth

Professional-grade polyphonic synthesizer built with C++ and iPlug2.

[![CI](https://github.com/ashokfernandez/PolySynth/actions/workflows/ci.yml/badge.svg)](https://github.com/ashokfernandez/PolySynth/actions/workflows/ci.yml)
[![PR Visual Regression](https://github.com/ashokfernandez/PolySynth/actions/workflows/visual-tests.yml/badge.svg)](https://github.com/ashokfernandez/PolySynth/actions/workflows/visual-tests.yml)
[![Latest Release](https://img.shields.io/github/v/release/ashokfernandez/PolySynth?display_name=tag)](https://github.com/ashokfernandez/PolySynth/releases/latest)
[![Web Demo](https://img.shields.io/badge/Web%20Demo-Live-blue)](https://ashokfernandez.github.io/PolySynth/web-demo/)

## Live Links

- Web demo: [https://ashokfernandez.github.io/PolySynth/web-demo/](https://ashokfernandez.github.io/PolySynth/web-demo/)
- Latest release downloads: [https://github.com/ashokfernandez/PolySynth/releases/latest](https://github.com/ashokfernandez/PolySynth/releases/latest)
- GitHub Actions: [https://github.com/ashokfernandez/PolySynth/actions](https://github.com/ashokfernandez/PolySynth/actions)

## Free Installation Guide

1. Download the latest release assets from [GitHub Releases](https://github.com/ashokfernandez/PolySynth/releases/latest).
2. macOS:
   Install the `.pkg` and, if prompted by Gatekeeper, right-click the installer and choose **Open** once.
   The installer clears plugin quarantine flags so DAWs can load PolySynth normally.
3. Windows:
   Unzip the VST3 bundle and copy `PolySynth.vst3` into your VST3 folder (usually `C:\Program Files\Common Files\VST3`).
   If SmartScreen appears, use **More info -> Run anyway** once.

## Feature Snapshot

- 5-voice polyphony with deterministic voice allocation
- Dual oscillators (saw, square, triangle, sine)
- Resonant low-pass filter
- ADSR envelopes (amp + filter)
- LFO modulation paths
- Preset management (factory + user presets)
- Cross-platform targets: desktop/plugin + web demos

## Quick Start

### Prerequisites

- CMake 3.14+
- `just`
- Node.js 22+ (for web demo workflows)
- Xcode (macOS) or MSVC (Windows)

### Setup + Build

```bash
git clone --recursive https://github.com/ashokfernandez/PolySynth.git
cd PolySynth

just deps
just build
just test
```

### Common Commands

```bash
just                  # list all tasks
just check            # lint + test
just ci-pr            # lint + asan + tsan + test
just desktop-rebuild  # rebuild + launch desktop app
just sandbox-build    # build native UI sandbox
just sandbox-run      # launch native UI sandbox
just vrt-run          # run native visual regression checks
just gallery-test     # sandbox-build + vrt-run
just web-demo         # build and run web demo
```

### Logs

All `just` tasks write full logs to `.artifacts/logs/<run-id>/`.

```bash
just logs-latest
POLYSYNTH_VERBOSE=1 just test
```

## Documentation Map

- Docs hub: [`/Users/ashokfernandez/Software/PolySynth/docs/README.md`](docs/README.md)
- Architecture overview: [`/Users/ashokfernandez/Software/PolySynth/docs/architecture/OVERVIEW.md`](docs/architecture/OVERVIEW.md)
- Design principles: [`/Users/ashokfernandez/Software/PolySynth/docs/architecture/DESIGN_PRINCIPLES.md`](docs/architecture/DESIGN_PRINCIPLES.md)
- Testing guide: [`/Users/ashokfernandez/Software/PolySynth/docs/architecture/TESTING_GUIDE.md`](docs/architecture/TESTING_GUIDE.md)
- Testing setup: [`/Users/ashokfernandez/Software/PolySynth/docs/guides/TESTING_SETUP.md`](docs/guides/TESTING_SETUP.md)
- Agent policy: [`/Users/ashokfernandez/Software/PolySynth/AGENTS.md`](AGENTS.md)
- Agent handbook: [`/Users/ashokfernandez/Software/PolySynth/docs/agents/README.md`](docs/agents/README.md)
- Planning hub: [`/Users/ashokfernandez/Software/PolySynth/plans/README.md`](plans/README.md)

## Testing

```bash
just test
just asan
just tsan
just sandbox-build
just vrt-run
just gallery-test
```

## License

See [`/Users/ashokfernandez/Software/PolySynth/LICENSE`](LICENSE).

## Acknowledgments

- [iPlug2](https://github.com/iPlug2/iPlug2)
- [Catch2](https://github.com/catchorg/Catch2)
- [nlohmann/json](https://github.com/nlohmann/json)
