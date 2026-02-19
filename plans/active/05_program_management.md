# 05. Program Management & Roadmap

## 1. Decision Making: ADRs
We will use **Architecture Decision Records** (ADRs) to document significant choices.
*   **Format**: `doc/adr/001-title.md`
*   **Content**: Context, Decision, Consequences.
*   **Trigger**: Any choice that is hard to reverse (e.g., choice of filter topology, use of SIMD library).

## 2. Iterative Roadmap

### Phase 1: The "Walking Skeleton"
*   **Goal**: A complete, compiled, tested pipeline that produces silence (or a sine wave).
*   **deliverables**:
    *   `polysynth/` repo structure.
    *   `src/core/Engine.h` (empty).
    *   `src/platform/desktop` (iPlug2 project).
    *   CI Workflow running.

### Phase 2: The "Bleep" (Monophonic)
*   **Goal**: First sound.
*   **Deliverables**:
    *   1x Oscillator (Sawtooth).
    *   1x VCA (Gate driven).
    *   MIDI Note On/Off handling.
    *   Golden Master test setup.

### Phase 3: The "Subtract" (Filter/Env)
*   **Goal**: The classic synth voice.
*   **Deliverables**:
    *   ADSR Envelopes.
    *   ZDF Lowpass Filter.
    *   Parameter smoothing.

### Phase 4: The "Poly" (Voice Manager)
*   **Goal**: Chords.
*   **Deliverables**:
    *   `VoiceManager` class.
    *   Voice allocation logic (polyphony limit 8).
    *   Voice stealing (oldest note).

### Phase 5: The "Prod" (UI & Polish)
*   **Goal**: Usability.
*   **Deliverables**:
    *   Vector UI (SVG).
    *   Preset Manager.
    *   Modulation Matrix UI.

## 3. Release Criteria
*   **Performance**: CPU < 5% per voice on M1 Air.
*   **Stability**: 0 Crashes in `auval`.
*   **Quality**: All regression tests pass.
