# PolySynth UI Strategy: Web-First with iPlug2

## Goal
Create a modern, responsive, and aesthetically pleasing UI for PolySynth using a "Web First" approach, leveraging iPlug2's `IWebView` allowing us to use standard web technologies (HTML/CSS/JS) for the interface.

## User Review Required
> [!IMPORTANT]
> **Technology Choice**: We are choosing `IWebView` over the native `IGraphics`. This introduces a dependency on the system's WebView (EdgeWebView2 on Windows, WebKit on macOS). This is generally acceptable for modern plugins but adds a slight runtime overhead compared to native drawing.

## Architecture

### 1. Hybrid Architecture
-   **DSP Engine (C++)**: Handles audio processing, MIDI, and parameter management (Source of Truth).
-   **UI Layer (Web)**: Hosted in an `IWebView`. Handles visualization and user interaction.
-   **Bridge**:
    -   **C++ -> JS**: `OnMessage` / `SendControlValue` to update UI when parameters change (automation, presets).
    -   **JS -> C++**: `IPVWebViewControl::SendControlValueFromDelegate` / `IPCC` to send parameter changes from UI to DSP.

### 2. Technology Stack
-   **Core**: Vanilla HTML5, CSS3, ES6+ JavaScript.
    -   **Why?**: No build step required for simple prototypes, high performance, easy integration.
    -   *Future Consideration*: adopt React/Svelte if UI complexity explodes.
-   **Styling**: CSS Variables for theming (Dark/Light mode), Flexbox/Grid for layout.
-   **Graphics**: SVG for crisp icons/knobs, Canvas API for real-time visualizers (Envelopes, Oscilloscopes).

## Development Workflow

1.  **Web Prototype (Current Phase)**
    -   Develop the UI as a standalone static site.
    -   Mock the C++ backend using a JS `MockBridge`.
    -   Iterate on design and UX in a standard browser (fast reload).

2.  **Integration**
    -   Place web assets in `resources/web`.
    -   Enable `IWebView` in iPlug2 `config.h`.
    -   Implement the C++ `IEditorDelegate` to serve files and handle messages.

3.  **Testing**
    -   **Browser**: Visual regression testing, interaction testing.
    -   **Plugin**: Verify parameter syncing and state recall.

## Component Design System
We will build a small library of reusable audio components:
-   `RotaryKnob`: SVG-based knob with drag interaction.
-   `Slider`: Vertical/Horizontal faders.
-   `EnvelopeGraph`: Interactive ADSR visualization (Canvas).
-   `Scope`: Real-time waveform display (Canvas).

## Next Steps
1.  Approve this strategy.
2.  Design the "Look and Feel" (Colors, Typography, Layout).
3.  Build the `MockBridge` and initial controls in the web prototype.
