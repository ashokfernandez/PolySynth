# 01. Vision & Strategy: "Project PolySynth"

## 1. Executive Summary
We are building a professional-grade, cross-platform polyphonic synthesizer.
*   **Core Technology**: iPlug2 (C++) + Custom "Hub & Spoke" DSP Engine.
*   **Primary Targets**: macOS (AU/VST3/CLAP), Windows (VST3/CLAP).
*   **Secondary Target**: Hardware Embedded Systems (Daisy, Elk, Bare Metal).
*   **Development Methodology**: "Spec-First" Agentic Workflow.

## 2. Key Differentiators
1.  **Strict Isolation**: The DSP Core is a pure C++ library with *zero* dependencies on iPlug2 or operating system calls. This ensures the code can run on a microcontroller just as easily as in a DAW.
2.  **Agentic DNA**: The project is structured specifically to be read, understood, and extended by AI agents. Code is "Prompt-Ready".
3.  **Quality First**: We treat audio software like avionics. No memory allocations in the audio thread. 100% deterministic rendering for automated regression testing.

## 3. The "Hybrid" Plan
We are combining the best aspects of two previous planning efforts:
*   **From "Antigravity"**: The "Hub & Spoke" architecture and the Agentic Persona system (@Architect, @DSPEngineer).
*   **From "Codex"**: The precise Prophet-5 inspired topology, the Detailed ADR (Architecture Decision Record) process, and the Golden Master testing strategy.

## 4. Immediate Goals (Phase 1)
*   **Skeleton**: A repo that compiles on Mac/Win CI actions.
*   **Core**: A "Gain" plugin that uses the separated DSP engine.
*   **Agent**: A proven "Spec-First" loop where an agent implements a simple feature (e.g., a Sine Oscillator) passing a headless test.

## 5. Risk Management
*   **Risk**: iPlug2 complexity with CLAP.
    *   *Mitigation*: We will verify CLAP builds in the very first Skeleton phase.
*   **Risk**: DSP Performance on Embedded.
    *   *Mitigation*: We use `sample_t` typedefs to strip `double` precision down to `float` where needed, and enforce "No STL in Audio Thread" rules via linter.
