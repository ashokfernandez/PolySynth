# 03. Agentic Workflow ("The Spec-First Loop")

## 1. Philosophy
We do not just ask AI to "write code". We use AI to simulate a specialized engineering team. Each persona has a strict role and constraint set.

## 2. The Personas (System Prompts)

### @Architect
*   **Role**: Lead Engineer.
*   **Output**: High-level C++ Header files (`.h`), Mermaid diagrams, Interface definitions.
*   **Constraint**: *Never* writes implementation code. Focuses on API design and separation of concerns.

### @DSPEngineer
*   **Role**: Audio Mathematician.
*   **Output**: The body of the functions (`.cpp`).
*   **Constraint**: *Must* explain the math (LaTeX) and optimize for CPU. *Must* ensure no heap allocations in audio path.

### @GUIArchitect
*   **Role**: Interface Designer.
*   **Output**: `IGraphics` layout code, SVG definitions.
*   **Constraint**: Focuses on usability and "feel". Connects via `std::atomic` or Queues to DSP.

### @TestEngineer
*   **Role**: QA & CI Specialist.
*   **Output**: Catch2 tests, GitHub Actions YAML, Headless render scripts.
*   **Constraint**: The "Adversary". Tries to break the code. Writes the test *before* the implementation (TDD).

## 3. The Implementation Loop
For every feature (e.g., "Add a Moog Ladder Filter"):

1.  **Specification (@Architect)**: 
    *   User requests feature.
    *   @Architect designs `LadderFilter.h` with inputs (Cutoff, Res) and outputs.
    *   @Architect updates `02_architecture.md` if needed.

2.  **Test Harness (@TestEngineer)**:
    *   Creates `tests/Test_LadderFilter.cpp`.
    *   Writes a case that instantiates the class and checks basic invariants (e.g., "energy doesn't explode").
    *   The test fails (Red).

3.  **Implementation (@DSPEngineer)**:
    *   Implements the math in `LadderFilter.cpp` to satisfy the test.
    *   Explains the topology (e.g., "Huovilainen model").
    *   The test passes (Green).

4.  **Integration (@GUIArchitect)**:
    *   Adds a knob to the UI.
    *   Connects the parameter ID.

    *   User listens to the build.
    *   User checks the code diff.

## 4. Environment & Cheat Sheet

### Running Unit Tests
Tests are the source of truth. Always run them before and after changes.

```bash
# Navigate to the test directory
cd polysynth/tests

# Create build directory if it doesn't exist
mkdir -p build && cd build

# Configure and Build
cmake .. -DIPLUG_UNIT_TESTS=ON
make

# Run Tests
./run_tests
```

### Generating Audio Demos
Use the demo executables to verify signal integrity.

```bash
# From polysynth/tests/build
./demo_render_wav
# Output: demo_*.wav (Check docs/audio/ or listen manually)
```

## 5. Concurrency & Coordination
To enable multiple agents to work in parallel without conflicts:

### 5.1 The "Ticket" System
1.  **Claim It**: Before starting, check `polysynth/plans/roadmap.md`. Find your Milestone.
2.  **Mark It**: Change `[ ] Milestone 1.1` to `[/] Milestone 1.1 (@AgentName)`.
3.  **Atomic Units**: Work on *one* bullet point at a time. Do not claim an entire Phase.

### 5.2 Branching Strategy
*   **Main Branch**: `main` is always stable (compiles + passes tests).
*   **Feature Branches**: Create a branch for your ticket: `user/feat/short-description`.
    *   Example: `agent/feat/adsr-envelope`
*   **Merging**:
    1.  Work on your branch.
    2.  `git fetch origin main`
    3.  `git rebase main` (Resolve conflicts locally).
    4.  Run `./run_tests.sh`.
    5.  Push and create PR (or merge if you have direct access).

### 5.3 Interface-First Contracts
If Agent A needs `Filter` (impl by Agent B):
1.  Agent A defines `Filter.h` with pure virtual methods.
2.  Agent A mocks `Filter` in their test.
3.  Agent B implements `Filter.cpp` against the interface.
4.  They merge.
This avoids "blocking" on implementation details.
