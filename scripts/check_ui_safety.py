#!/usr/bin/env python3
import os
import re
import sys

# Path to PolySynth.cpp
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
source_file = os.path.join(project_root, "src/platform/desktop/PolySynth.cpp")

print(f"Checking {source_file} for unsafe UI drawing in OnLayout...")

with open(source_file, 'r') as f:
    content = f.read()

# Find direct drawing calls which are unsafe in OnLayout

unsafe_patterns = [
    r"g->FillRect\(",
    r"g->DrawRect\(",
    r"g->DrawText\("
]

# In PolySynth.cpp, all drawing should be happening inside Controls.
# ANY occurrence of `g->Draw` or `g->Fill` in `PolySynth.cpp` is suspicious.

errors = []
lines = content.split('\n')
for i, line in enumerate(lines):
    for pattern in unsafe_patterns:
        if re.search(pattern, line):
            # Check if it's commented out
            if line.strip().startswith("//"):
                continue
            errors.append(f"Line {i+1}: Found potentially unsafe direct drawing: {line.strip()}")

if errors:
    print("FAILED: Found unsafe direct drawing calls in PolySynth.cpp:")
    for e in errors:
        print(e)
    print("Direct drawing (FillRect, DrawText, etc.) on the IGraphics context in layout code causes crashes.")
    print("Please use IControl (e.g., IPanelControl, ITextControl) instead.")
    sys.exit(1)
else:
    print("SUCCESS: No unsafe direct drawing calls found.")
    sys.exit(0)
