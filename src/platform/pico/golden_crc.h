#pragma once
// Expected CRC32 of a 256-frame I2S buffer rendered with default patch + A4.
// This value is checked during the firmware self-test to catch regressions in
// the complete Pico signal chain (Engine → gain → fast_tanh → SSAT → I2S pack).
//
// To regenerate: build with -DGOLDEN_CRC_GENERATE, run in Wokwi or on
// hardware, read the CRC from serial output, and update this value.
//
// The test renders 256 stereo frames (512 uint32_t words) starting from a
// freshly-initialized engine with default SynthState + note A4 vel 100.
#define PICO_GOLDEN_CRC32 0x00000000  // placeholder — set after first run
