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

# Find OnLayout function body
# Simple regex to find start of OnLayout and some reasonable amount of lines or matching braces
# For now, just searching the file, assuming OnLayout is the only place we care about layout logic
# Actually, BuildHeader and BuildFooter are also layout functions.

unsafe_patterns = [
    r"g->FillRect\(",
    r"g->DrawRect\(",
    r"g->DrawText\("
]

# We want to forbid these ONLY in the layout construction phase if they are not inside a Control's Draw method or IGraphics::Draw
# But simple grep finds them everywhere.
# However, in PolySynth.cpp, all drawing should be happening inside Controls.
# The only exception is if we are implementing a custom control inline? 
# But we moved custom controls to headers.
# So PolySynth.cpp should ideally NOT have direct Draw/Fill calls on 'g' except maybe in ProcessMidiMsg? No.
# PolySynth.cpp is the plugin class. It shouldn't be drawing directly. IControl/IGraphics does drawing.
# Wait, `OnLayout` attaches controls. `Draw` methods of controls draw.
# If `PolySynth.cpp` has `g->Draw...`, it's likely a mistake (like the one I fixed).
# Unless it implements `Draw` for a control defined in the file.
# But I removed local classes.
# So ANY occurrence of `g->Draw` or `g->Fill` in `PolySynth.cpp` is suspicious now.

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
