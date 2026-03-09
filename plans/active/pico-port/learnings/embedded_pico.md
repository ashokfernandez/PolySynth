# Embedded Pico Learnings (Sprint Pico-1)

## Hard-Won Learnings

| # | Learning | Detail |
|---|----------|--------|
| 1 | `<atomic>` just works on RP2350 | GCC 15.2 on Cortex-M33 handles `std::atomic<int>`, `std::atomic<uint64_t>` natively. No `-latomic` link flag needed. Was flagged as a known risk — resolved cleanly. |
| 2 | Toolchain discovery > hardcoded paths | Original plan hardcoded `/Applications/ARM/bin` (GCC 10.3). Machine had two installs. `scripts/find_arm_toolchain.sh` with version validation prevents this class of bug. |
| 3 | `build/pico` already in `.gitignore` | The `build*/` glob pattern covers it. Plan incorrectly assumed it wasn't covered. |
| 4 | `picotool reboot` false-positive error | After flash, `picotool reboot` reports `ERROR: rebooting` because USB drops on device reboot. Flash succeeds. Don't chase this. |
| 5 | No CMakeLists.txt changes needed for Engine.h | Include path to `src/core` and SEA_DSP/SEA_Util links were already configured. Just adding `#include "Engine.h"` was enough. |
| 6 | `-fno-rtti` warns on C files | Pico SDK C sources trigger `'-fno-rtti' is valid for C++/D/ObjC++ but not for C`. Harmless but noisy. Future fix: scope to `$<COMPILE_LANGUAGE:CXX>` generator expression. |
| 7 | Pico SDK download script must run before toolchain discovery | `pico-build` calls `download_pico_sdk.sh` first, then `find_arm_toolchain.sh`. Order matters because CMake configure needs both SDK and toolchain. |

## Sprint Pico-1 Results

- Full DSP header chain (`Engine.h` → VoiceManager → all SEA_DSP) compiles for ARM with zero code changes
- 541.5K UF2, confirmed on hardware (LED + serial output)
- `<atomic>` works natively on Cortex-M33 with GCC 15.2
- Toolchain discovery script is reusable by CI
