# Sprint Retrospective Notes

## Catching real-time audio thread issues earlier
- Add a regression test that inspects `PolySynthPlugin::ProcessBlock` and fails if `printf(` appears in the callback body.
- Add a PR checklist item: "No blocking I/O, allocation, locks, or logging in DSP/audio callbacks."
- During review, always scan callback paths (`ProcessBlock`, per-sample `Process`) for non-RT-safe operations.

## Catching legato/unison pitch bugs earlier
- Add dedicated regression coverage for legato retriggers with persistent unison detune to ensure pitch does not compound over repeated note-ons.
- Add design note to keep pitch math in one canonical path: derive target pitch from base note + detune, then apply glide toward that target.
- For voice/pitch changes, include at least one test that verifies final settled pitch numerically against expected frequency.
