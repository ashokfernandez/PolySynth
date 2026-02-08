# PolySynth

**PolySynth** is a professional-grade, cross-platform polyphonic synthesizer built with C++ and iPlug2. It features a custom "Platform Agnostic" DSP engine designed for high performance and portability, targeting macOS (AU/VST3/APP) and Windows, with future embedded hardware support.

## 🌟 Features

*   **Custom DSP Engine**: A pure C++ audio engine, decoupled from any specific framework.
*   **Polyphony**: Supports up to 8 voices with efficient voice allocation.
*   **Oscillators**: Multi-waveform oscillators (Sawtooth, Square, Triangle, Sine) with band-limited synthesis.
*   **Filters**: Resonant Low-Pass Filter (LPF) with cutoff and resonance control.
*   **Envelopes**: ADSR (Attack, Decay, Sustain, Release) envelopes for amplitude and filter modulation.
*   **Cross-Platform**: Builds as a VST3, Audio Unit, and Standalone App for macOS and Windows.
*   **Headless Testing**: Comprehensive unit testing and audio artifact generation without requiring a DAW.

## 📚 Documentation & Architecture

The project follows a strict "Hub & Spoke" architecture:

*   [**Vision & Strategy**](plans/01_vision_and_strategy.md): High-level goals and "Hybrid" approach.
*   [**Architecture**](plans/02_architecture.md): Technical design and component breakdown.
*   [**Agentic Workflow**](plans/03_agentic_workflow.md): Development process using AI personas.
*   [**Testing Strategy**](plans/04_testing_strategy.md): "Headless" and "Golden Master" testing methodologies.

## 🎧 Audio Demos

We automatically generate audio samples from our test suite on every commit to ensure DSP correctness.
[**Listen to the latest Audio Artifacts & Test Report**](https://ashokfernandez.github.io/PolySynth/)

## 🏗 Directory Structure

*   `src/core/`: **The Engine**. Pure C++ DSP code. No framework dependencies.
*   `src/platform/`: **The Wrapper**. iPlug2 and hardware integration layers.
*   `tests/`: **The Verification**. Catch2 unit tests and offline rendering tools.
*   `plans/`: **The Brain**. extensive project documentation and architectural decision records (ADRs).
*   `scripts/`: Utility scripts for building dependencies and generating reports.

## 🚀 Getting Started

### Prerequisites
*   **CMake**: 3.14 or later.
*   **Git**: For version control.
*   **C++ Compiler**: Clang (macOS) or MSVC (Windows).

### Building
1.  **Clone the repository**:
    ```bash
    git clone https://github.com/ashokfernandez/PolySynth.git
    cd PolySynth
    ```
2.  **Download Dependencies**:
    ```bash
    ./polysynth/scripts/download_dependencies.sh
    ```
3.  **Build (Headless Tests)**:
    ```bash
    cd polysynth/tests
    mkdir build && cd build
    cmake ..
    make
    ./run_tests
    ```

## 🤝 Contributing

Please read the [Vision & Strategy](plans/01_vision_and_strategy.md) before contributing. All changes must be verified with headless tests.

---
*Built with ❤️ by Ashok & Antigravity*
