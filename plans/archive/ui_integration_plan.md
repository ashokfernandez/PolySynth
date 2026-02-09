# UI Integration Plan: Web to C++

## Goal
Integrate the `polysynth/web` prototype into the PolySynth iPlug2 C++ project using `IWebView`.

## Prerequisites
-   iPlug2 `IGraphics` dependency must be disabled or configured for WebView.
-   Platform webview runtimes (EdgeWebView2 on Windows, WebKit on macOS) are assumed present.

## Implementation Steps

### 1. Project Configuration (`config.h`)
-   Define `WEBVIEW_EDITOR_DELEGATE` to enable the WebView infrastructure.
-   Ensure `NO_IGRAPHICS` is *not* defined if we want to mix, but likely we will replace the default editor.
    -   *Strategy*: We will use `IGraphics` as the container but use `IWebViewControl` to host the content, OR use a pure `IWebViewEditorDelegate`.
    -   *Decision*: Use `IWebViewEditorDelegate` for a pure web UI approach as it is cleaner.

### 2. Resource Management
-   **Bundling**: Web assets (`index.html`, `style.css`, `main.js`) need to be bundled into the plugin resources.
-   **macOS**: Add files to the App Bundle Resources.
-   **Windows**: Add files to the Resource script (`.rc`).

### 3. C++ Implementation
-   **Class**: Create `PolySynthEditorDelegate` inheriting from `IWebViewEditorDelegate`.
-   **Initialization**:
    -   In `OnAttach()`, load `index.html`.
-   **Communication (The Bridge)**:
    -   **C++ -> JS**: Override `OnParamChange(int paramIdx)` to call `SendControlValueFromDelegate(paramIdx, value)`.
    -   **JS -> C++**: The JS `window.PolySynthBridge` will communicate via the native message passing scheme provided by iPlug2 headers.

### 4. JavaScript Adaptation
-   Replace the `MockBridge` in `main.js` with the actual iPlug2 communication interface.
-   Ensure `SendControlValue` calls match the iPlug2 expect format.

## Proposed File Structure Changes
```text
polysynth/
├── resources/
│   └── web/
│       ├── index.html
│       ├── style.css
│       └── main.js
├── src/
│   └── PolySynthEditorDelegate.h  <-- NEW
```

## Verification
-   Build the plugin (APP/VST3).
-   Run in host (Reaper/Ableton).
-   Verify bidirectional control:
    -   Moving UI knob changes DSP param.
    -   Automating DSP param moves UI knob.
