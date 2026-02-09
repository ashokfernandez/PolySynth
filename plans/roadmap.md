# PolySynth Master Roadmap

This document outlines the strategic epics for PolySynth evolution. Each epic focuses on a major pillar of the instrument: Sonic Power, Visual Excellence, and Professional Reliability.

## Epic 1: Advanced Synthesis Core ✅
**Goal:** Expand the sonic palette beyond basic virtual analog.
*   [x] **Waveform Expansion**: Pure Sine, Triangle, and Band-limited Pulse Width Modulation (PWM).
*   [x] **Dual Oscillator Architecture**: Independent waveforms for Osc A/B with mixer and detune.
*   [x] **Complex Modulation**: Flexible LFO routing (Pitch, Filter, Amp) and dedicated Filter ADSR.
*   [x] **Advanced Demo Suite**: Comprehensive audio verification for all synthesis features.

## Epic 2: Virtual Analog (VA) Filter Suite (In Progress)
**Goal:** Implement industry-standard filter models using Topology-Preserving Transforms (TPT).
*   [ ] **Milestone 2.1: TPT Integrator Core**: Implement the fundamental SVF structure.
*   [ ] **Milestone 2.2: Moog-style Ladder**: 24dB resonant ladder filter with nonlinear saturation.
*   [ ] **Milestone 2.3: Sallen-Key (SKF)**: MS-20 style scream and character.
*   [ ] **Milestone 2.4: State Variable (SVF)**: Multi-mode (LP/HP/BP/Notch) with stable resonance.

## Epic 3: High-Performance Web UI 🚀
**Goal:** A "Wowed at first glance" interface using modern web technologies.
*   [ ] **Vite + React Migration**: Move from basic JS to a modern development workflow.
*   [ ] **Glassmorphism Design**: High-end aesthetic with semi-transparent panels and vibrant accents.
*   [ ] **Interactive Visualizers**: Real-time waveform and spectrum displays.
*   [ ] **Responsive Layout**: Fluid UI that scales from small plugin windows to 4K displays.

## Epic 4: Effects Engine 🌈
**Goal:** Integrated signal processing to create finished, mix-ready sounds.
*   [ ] **Stereo Delay**: Tempo-synced delays with feedback filtering.
*   [ ] **Dimensional Chorus**: BBD-style thickening and width.
*   [ ] **Plate/Hall Reverb**: Lush spatialization for pads and leads.
*   [ ] **Master Limiter**: Transparent protection against clipping.

## Epic 5: Preset System & Browser 📂
**Goal:** Effortless sound discovery and management.
*   [ ] **Advanced Preset Browser**: Category/Tag-based searching.
*   [ ] **Cloud Sync (Future)**: Sharing presets between sessions and devices.
*   [ ] **User Save/Load**: Persistent storage of custom sounds in JSON format.

## Epic 6: Professional Distribution 📦
**Goal:** Seamless installation and platform compatibility.
*   [ ] **VST3/AU Validation**: Passing Steinberg and Apple validation suites.
*   [ ] **Universal Installers**: Pkg (macOS) and MSI (Windows) with signing/notarization.
*   [ ] **CI/CD Deployment**: Automatic builds for every release tag.

---

## Technical Debt & Maintenance
*   [ ] **SIMD Optimization**: Vectorizing oscillator and filter loops for lower CPU usage.
*   [ ] **Internal Oversampling**: 2x/4x oversampling option for high-resonance filtering.
*   [ ] **Documentation**: Complete API docs for the Core DSP engine.
