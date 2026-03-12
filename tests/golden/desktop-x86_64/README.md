# Golden Audio Fixtures

This directory holds golden WAV fixtures generated from the PolySynth demo
executables. Use `scripts/golden_master.py --generate` to create them locally
and `scripts/golden_master.py --verify` to compare newly generated output
against the golden masters.

Note: Golden WAV files are tracked in git to allow the CI to verify the 
DSP output against a known baseline. If the DSP logic changes intentionally, 
regenerate these locally using `scripts/golden_master.py --generate` and commit 
 the updated WAV files.

