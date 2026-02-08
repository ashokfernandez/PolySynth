# Golden Audio Fixtures

This directory holds golden WAV fixtures generated from the PolySynth demo
executables. Use `scripts/golden_master.py --generate` to create them locally
and `scripts/golden_master.py --verify` to compare newly generated output
against the golden masters.

Note: WAV files are intentionally ignored in git because the PR system cannot
review binary assets. Generate them locally when needed.
