---
description: How to build and open the PolySynth desktop application
---

To build and open the local standalone build of PolySynth:

// turbo
1. To do a clean build and launch immediately:
```bash
./rebuild_and_run.sh
```

2. To just launch the existing build:
```bash
./run_desktop.sh
```

3. If you need to run the command-line version specifically for debugging:
```bash
./build_desktop/out/Debug/PolySynth.app/Contents/MacOS/PolySynth
```

4. To view the underlying Web UI in a browser (for design/debugging):
```bash
open src/platform/desktop/resources/web/index.html
```
