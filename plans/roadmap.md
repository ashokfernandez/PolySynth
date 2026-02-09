# PolySynth Master Roadmap: The Prophet-5 Evolution

This document defines the strategic roadmap for PolySynth, scaling from a foundational demo into a professional-grade Virtual Analog synthesizer inspired by the Sequential Circuits Prophet-5 topology.

---

## Phase 1: The Prophet Core (Foundation) ✅
**Goal:** Implement the "Digital Control Brain" and the basic "Polyphonic Voice Engine".

### Epic 1.1: The Brain (State Management) ✅
*   [x] **Single Source of Truth (SSOT)**: Implement `SynthState` pod to hold all active parameters. (Hint: UI updates State; State updates Engine).
*   [x] **Preset Library (NVRAM)**: Implement `PresetManager` for JSON serialization/deserialization.
*   [x] **The Edit Buffer**: Implement active state syncing between the Web UI and the C++ Engine.

### Epic 1.2: The Conductor (Voice Allocation) ✅
*   [x] **Voice Allocation Logic**: Implement 8-voice polyphony with Round-Robin and Oldest-Voice Stealing.
*   [x] **MIDI Event Loop**: Strictly decouple MIDI handling from the audio-rate voice processing.
*   [x] **Global CV Logic**: Implement the shared modulation bus (LFO/Mod Wheel) that feeds all voices.

### Epic 1.3: The Sound Generator (DSP Architecture) ✅
*   [x] **Unified Voice Class**: Create a re-instantiable `Voice` container holding Oscillators, Filter, and Envelopes.
*   [x] **Dual Oscillator Architecture**: Independent waveforms (Saw, Square, Triangle, Sine) for VCO A and VCO B.
*   [x] **Integrated Mixer**: Blending stage for VCO A/B before hitting the Filter stage.
*   [x] **Demos & Verification**: Created `demo_waveforms`, `demo_osc_mix`, and `demo_engine_poly`.

---

## Phase 2: High-Fidelity Signal Path (In Progress)
**Goal:** Implement the "Analog Soul" - VA Filters and Audio-Rate Modulation.

### Epic 2.1: Virtual Analog (VA) Filter Suite
*   [ ] **TPT Integrator Core**: Implement Topology-Preserving Transforms for alias-free stable filters.
*   [ ] **Moog-style Ladder filter**: 24dB resonant ladder with nonlinear saturation.
*   [ ] **Prophet-style 4-Pole**: Selectable 12dB/24dB models with stable resonance behavior.
*   [x] **Verification**: Created `demo_filter_resonance` and `demo_filter_envelope`.

### Epic 2.2: Poly-Mod Matrix (Audio-Rate)
*   [ ] **VCO B to VCO A Frequency**: Implement audio-rate pitch modulation (FM).
*   [ ] **VCO B to Filter Cutoff**: Implement audio-rate filter modulation.
*   [ ] **Envelope to VCO A Width**: Poly-mod logic for PWM shaping.

---

## Phase 3: Premium User Experience (In Progress)
**Goal:** A "Wowed at first glance" interface and modern workflow.

### Epic 3.1: Modern UI Infrastructure
*   [ ] **Vite + React Migration**: Move existing UI to a professional React workflow (In Progress).
*   [ ] **Bidirectional Param Sync**: Hardened bridge between React components and `SynthState`.
*   [ ] **Developer Testing**: Integrate Vitest for UI logic and component verification.

### Epic 3.2: High-End Aesthetics
*   [ ] **Glassmorphism Design**: Semi-transparent panels with vibrant gradients and "Prophet" wood-end accents.
*   [ ] **Interactive Visualizers**: Real-time FFT spectrum and Oscilloscope views using Canvas/WebGL.
*   [ ] **Responsive Layout**: Fluid scaling for 4K displays and small plugin windows.

---

## Phase 4: Expansion & Effects
**Goal:** Post-processing and additional sonic character.

### Epic 4.1: The FX Engine
*   [ ] **Dimensional Chorus**: BBD-style thickening.
*   [ ] **Stereo Delay**: Tempo-synced feedback delays.
*   [ ] **Master Limiter**: Transparent protection stage (Hint: Use look-ahead buffer for zero digital clipping).

---

## Phase 5: Professional Release
**Goal:** Distribution-ready artifacts and industry validation.

*   [ ] **Validation Suite**: Passing `auval` (Apple) and Steinberg VST3 validator.
*   [ ] **Installers**: DMG (macOS) and MSI (Windows) with signing and notarization.
*   [ ] **Automated Release**: CI/CD pipeline that builds installers on every release tag.

---

## Maintenance & Tech Debt
*   [ ] **SIMD Optimization**: Vectorizing VCO core loops.
*   [ ] **Oversampling**: 2x/4x switchable oversampling for the Filter stage.
*   [ ] **Docs**: Complete Doxygen documentation for the Core DSP engine.
