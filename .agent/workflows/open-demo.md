---
description: How to build and open the PolySynth desktop application
---

To build and open the local standalone build of PolySynth:

// turbo
1. To do a clean build and launch immediately:
```bash
just desktop-rebuild
```

2. To just launch the existing build:
```bash
just desktop-run
```

3. If you need to run the command-line version specifically for debugging:
```bash
./build_desktop/out/Debug/PolySynth.app/Contents/MacOS/PolySynth
```
