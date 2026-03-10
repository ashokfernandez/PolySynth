#pragma once

#include <cstdint>

class PicoSynthApp;

namespace pico_self_test {

// Run boot self-tests. Returns true if all passed.
// Emits [TEST:ALL_PASSED] or [TEST:SOME_FAILED] (Wokwi CI depends on these markers).
// audioCallback: the static trampoline function for I2S init test.
bool RunAll(PicoSynthApp& app, void (*audioCallback)(uint32_t*, uint32_t));

} // namespace pico_self_test
