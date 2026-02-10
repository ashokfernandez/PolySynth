# PolySynth

<p align="center">
  <strong>A professional-grade, cross-platform polyphonic synthesizer</strong><br>
  <em>Built with C++ and iPlug2 for maximum performance and portability</em>
</p>

<p align="center">
  <a href="https://github.com/ashokfernandez/PolySynth/actions"><img src="https://github.com/ashokfernandez/PolySynth/workflows/CI/badge.svg" alt="CI Status"></a>
  <a href="https://ashokfernandez.github.io/PolySynth/"><img src="https://img.shields.io/badge/Audio%20Demos-Listen%20Now-blue" alt="Audio Demos"></a>
</p>

---

## Overview

PolySynth is a classic-style virtual analog synthesizer designed for musicians, producers, and audio developers. It features a clean signal path with hands-on control over every parameter, inspired by the golden era of analog polysynths.

### Key Features

- **5-Voice Polyphony** — Rich, stackable voices with intelligent voice allocation
- **Dual Oscillators** — Sawtooth, Square, Triangle, and Sine with mix control
- **Resonant Filter** — 24dB/oct low-pass filter with self-oscillating resonance
- **ADSR Envelopes** — Dedicated amplitude and filter envelopes
- **LFO Modulation** — Multiple waveforms for vibrato, tremolo, and filter sweeps
- **Preset System** — Factory sounds + user preset save/load
- **Cross-Platform** — macOS (AU/VST3/Standalone), Windows (VST3)

---

## Architecture

PolySynth follows a **Hub & Spoke** architecture that cleanly separates concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                         Platform                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Desktop   │  │   Plugin    │  │      Embedded       │  │
│  │  (iPlug2)   │  │ (AU/VST3)   │  │   (Future: Daisy)   │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
└─────────┼────────────────┼────────────────────┼─────────────┘
          │                │                    │
          └────────────────┼────────────────────┘
                           │
          ┌────────────────▼────────────────┐
          │           Core Engine           │
          │  ┌──────────────────────────┐   │
          │  │       SynthState         │   │  ← Single Source of Truth
          │  │   (All Parameters)       │   │
          │  └────────────┬─────────────┘   │
          │               │                 │
          │  ┌────────────▼─────────────┐   │
          │  │     Voice Manager        │   │  ← Polyphony & Allocation
          │  │  ┌─────┐ ┌─────┐ ┌─────┐ │   │
          │  │  │ V1  │ │ V2  │ │ ... │ │   │
          │  │  └──┬──┘ └──┬──┘ └──┬──┘ │   │
          │  └─────┼───────┼───────┼────┘   │
          │        └───────┼───────┘        │
          │                ▼                │
          │  ┌──────────────────────────┐   │
          │  │     DSP Components       │   │
          │  │  • Oscillators (BLIT)    │   │
          │  │  • Filters (VA/Biquad)   │   │
          │  │  • Envelopes (ADSR)      │   │
          │  │  • LFO                   │   │
          │  └──────────────────────────┘   │
          └─────────────────────────────────┘
```

### Design Principles

| Principle | Implementation |
|-----------|----------------|
| **Platform Agnostic** | Core DSP has zero framework dependencies |
| **Single Source of Truth** | `SynthState` struct holds all parameters |
| **Testable** | Headless rendering without DAW required |
| **Real-time Safe** | No allocations in audio thread |

---

## Project Structure

```
PolySynth/
├── src/
│   ├── core/                    # Pure C++ DSP Engine
│   │   ├── dsp/                 # DSP building blocks
│   │   │   ├── Oscillator.h     # Band-limited oscillators
│   │   │   ├── ADSREnvelope.h   # Envelope generator
│   │   │   ├── BiquadFilter.h   # IIR filter
│   │   │   └── va/              # Virtual Analog filters (TPT)
│   │   ├── Engine.h             # Main synthesis engine
│   │   ├── Voice.h              # Single voice (osc + env + filter)
│   │   ├── VoiceManager.h       # Polyphony & allocation
│   │   ├── SynthState.h         # Central state struct
│   │   └── PresetManager.h      # JSON preset I/O
│   │
│   └── platform/
│       └── desktop/             # iPlug2 wrapper
│           ├── PolySynth.cpp    # Plugin entry point
│           ├── PolySynth_DSP.h  # DSP ↔ Plugin bridge
│           └── resources/
│               └── web/         # React UI
│
├── tests/
│   ├── unit/                    # Catch2 unit tests
│   └── demos/                   # Audio rendering demos
│
└── external/
    └── iPlug2/                  # Audio plugin framework
```

---

## Getting Started

### Prerequisites

- **CMake** 3.14+
- **Node.js** 16+ (for UI development)
- **Xcode** (macOS) or **MSVC** (Windows)

### Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/ashokfernandez/PolySynth.git
cd PolySynth

# Download iPlug2 dependencies
./scripts/download_dependencies.sh

# Build and run tests
cd tests && mkdir build && cd build
cmake .. && make
./run_tests

# Build desktop app (macOS)
cd ../../src/platform/desktop
cmake -B build
cmake --build build --target PolySynth-app

# Launch the app
open ~/Applications/PolySynth.app
```

### Component Gallery Quick Commands

Requires Node 22+ for Storybook 10 in `tests/Visual`.
If you use `nvm`, run `nvm use` from repo root (`.nvmrc` is set to Node 22.22.0).

```bash
# Build gallery plugin output (WAM/web assets)
./build_gallery.sh

# Rebuild gallery (clean build)
./rebuild_gallery.sh

# View gallery in Storybook (localhost:6006 by default)
./view_gallery.sh

# Rebuild + immediately view
./rebuild_and_view_gallery.sh

# Build static gallery docs page (for docs/component-gallery/index.html)
./build_gallery_pages.sh

# View static gallery docs over HTTP (required; file:// is blocked by browser CORS)
./view_gallery_pages.sh

# Build + run visual regression tests
./test_gallery_visual.sh

# Run Storybook Vitest checks only
cd tests/Visual && npm run test:stories
```

---

## Audio Demos

Every commit generates audio artifacts to verify DSP correctness:

🎧 **[Listen to the latest demos →](https://ashokfernandez.github.io/PolySynth/)**

---

## Testing

PolySynth uses a comprehensive testing strategy:

| Test Type | Description | Count |
|-----------|-------------|-------|
| **Unit Tests** | Component-level DSP verification | 30 cases |
| **Golden Master** | WAV comparison for regression detection | Automated |
| **UI Tests** | React component testing | 16 cases |
| **Integration** | Full build verification | CI/CD |

Run the test suite:

```bash
# C++ unit tests
./tests/build/run_tests

# JavaScript tests
cd src/platform/desktop/resources/web
npm test
```

---

## Factory Presets

| Preset | Character | Key Settings |
|--------|-----------|--------------|
| 🎹 **Warm Pad** | Soft, evolving | Slow attack, low cutoff |
| ⚡ **Bright Lead** | Punchy, present | Fast attack, high resonance |
| 🎸 **Dark Bass** | Deep, growling | Very low cutoff, LFO modulation |

---

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Write tests for your changes
4. Ensure all tests pass (`./tests/build/run_tests`)
5. Submit a pull request

Please read the [Architecture documentation](.agent/rules/architecture.md) before contributing.

---

## License

This project is provided for educational and personal use. See [LICENSE](LICENSE) for details.

---

## Acknowledgments

Built with these excellent open-source projects:

- [iPlug2](https://github.com/iPlug2/iPlug2) — Cross-platform audio plugin framework
- [Catch2](https://github.com/catchorg/Catch2) — C++ testing framework
- [nlohmann/json](https://github.com/nlohmann/json) — JSON for Modern C++
- [Vite](https://vitejs.dev/) + [React](https://react.dev/) — Modern web UI toolkit
