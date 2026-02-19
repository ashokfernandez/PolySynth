# DSP Refactor Plan: Side Effects Audio (SEA)

## Goal
Re-platform the legacy PolySynth DSP into a reusable, dual-target core library ("Side Effects Audio") that supports both Desktop (High Precision) and Embedded (Low Latency) targets.

## Phase 1: Foundation & Setup
**Objective**: Establish the library structure, platform abstraction, and third-party dependencies.

### 1.1 Directory Structure
Create a new directory structure for the core library:
```text
/libs
  /SE_DSP
    /include
      sea_platform.h       <-- PAL
      sea_math.h           <-- Math facade
      sea_oscillator.h     <-- Component facade
      sea_env.h
      sea_filter_ladder.h
      sea_filter_svf.h
      sea_utils.h          <-- Internal helpers (TPTIntegrator refactored)
    /src
      sea_oscillator.cpp
      sea_env.cpp
      sea_utils.cpp
      CMakeLists.txt
```

### 1.2 CMake Configuration
Create `libs/SE_DSP/CMakeLists.txt` to handle the dual-target build and DaisySP dependency.

```cmake
cmake_minimum_required(VERSION 3.15)
project(SideEffectsAudio)

# --- Dual-Target Configuration ---
set(SEA_PLATFORM_TARGET "DESKTOP" CACHE STRING "Components: DESKTOP or EMBEDDED")

if(SEA_PLATFORM_TARGET STREQUAL "EMBEDDED")
    add_compile_definitions(SEA_PLATFORM_EMBEDDED)
    message(STATUS "SEA: Building for EMBEDDED (Float/FastMath)")
else()
    add_compile_definitions(SEA_PLATFORM_DESKTOP)
    message(STATUS "SEA: Building for DESKTOP (Double/Precision)")
endif()

# --- Dependencies (DaisySP) ---
include(FetchContent)
FetchContent_Declare(
    daisysp
    GIT_REPOSITORY https://github.com/electro-smith/DaisySP.git
    GIT_TAG        master # Or a specific stable commit
)
FetchContent_MakeAvailable(daisysp)

# --- Library Definition ---
add_library(SE_DSP STATIC
    src/sea_oscillator.cpp
    src/sea_env.cpp
    # Add other sources here
)

# Standard Include Paths
target_include_directories(SE_DSP PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# DaisySP is PRIVATE to ensure isolation
target_link_libraries(SE_DSP PRIVATE daisysp)
# We might need to expose daisysp headers internally but not publicly
target_include_directories(SE_DSP PRIVATE ${daisysp_SOURCE_DIR})
```

### 1.3 Platform Abstraction Layer (PAL)
Create `libs/SE_DSP/include/sea_platform.h`:

```cpp
#pragma once

#include <cmath>
#include <cstdint>

namespace sea {

#if defined(SEA_PLATFORM_EMBEDDED)
    // --- EMBEDDED MODE (Float, Opt-for-Speed) ---
    using Real = float;
    #define SEA_INLINE __attribute__((always_inline)) inline
    
    // Fast approx macros if needed, or map to internal fast math
    #define SEA_FAST_MATH
    
#else
    // --- DESKTOP MODE (Double, High-Precision) ---
    using Real = double;
    #define SEA_INLINE inline

#endif

// Common Constants
constexpr Real kPi = static_cast<Real>(3.14159265358979323846);
constexpr Real kTwoPi = static_cast<Real>(6.28318530717958647692);

} // namespace sea
```

### 1.4 Math Facade (sea_math.h)
Implement static wrappers.

```cpp
#pragma once
#include "sea_platform.h"
#include <cmath>

namespace sea {
class Math {
public:
    static SEA_INLINE Real Sin(Real x) { return std::sin(x); }
    static SEA_INLINE Real Cos(Real x) { return std::cos(x); }
    static SEA_INLINE Real Tan(Real x) { return std::tan(x); }

    static SEA_INLINE Real Tanh(Real x) {
#ifdef SEA_FAST_MATH
        // Fast approximation for embedded: x / (1 + |x|)
        // Or simpler Pade approx
        // Example: simple sigmoid-like approx for speed
        Real x2 = x * x;
        return x * (27.0 + x2) / (27.0 + 9.0 * x2); 
#else
        return std::tanh(x);
#endif
    }
};
}
```

## Phase 2: Core Refactor (TPT Filters)
**Objective**: Port existing TPT filters (`VALadderFilter`, `VASVFilter`) to the `sea` namespace and template them.

### 2.1 Refactor Utility
Refactor `TPTIntegrator` inside `libs/SE_DSP/include/sea_utils.h` as `sea::TPTIntegrator<T>`.
- **Change**: Replace `double` with template `T`.
- **Change**: Use `sea::Math` functions.

### 2.2 Port VALadderFilter
Create `libs/SE_DSP/include/sea_filter_ladder.h`:
- Rename class to `sea::LadderFilter<typename T>`.
- Replace all `double` with `T`.
- Replace `std::tanh` with `sea::Math::Tanh`.
- Ensure `Init()` allocates no memory.

### 2.3 Port VASVFilter
Create `libs/SE_DSP/include/sea_filter_svf.h`:
- Rename class to `sea::SVFFilter<typename T>`.
- Apply same templating and math wrapper changes.

## Phase 3: Facade Implementation
**Objective**: Wrap functionality (DaisySP) without exposing it.

### 3.1 Oscillator Facade
**Header (`sea_oscillator.h`)**:
```cpp
namespace sea {
class Oscillator {
public:
    void Init(Real sampleRate);
    void SetFreq(Real freq);
    Real Process();
private:
    class Impl;
    // PIMPL idiom or aligned storage to hide types
    alignas(16) uint8_t impl_storage_[64]; // Size large enough for daisysp::BlOsc
};
}
```
**Implementation (`sea_oscillator.cpp`)**:
- Include `daisysp.h`.
- Use placement new in `Init` to create the Daisy oscillator inside `impl_storage_`.
- `Process` casts `impl_storage_` back to Daisy type and calls its process.

## Phase 4: Verification
**Objective**: Ensure correctness and performance.

1.  **Golden Master Test**:
    - Write a standalone test `tests/filter_test.cpp`.
    - Run the OLD `VALadderFilter` and the NEW `sea::LadderFilter<double>` side-by-side.
    - Feed them the same noise bursts and sweeps.
    - Assert output delta < 1e-9.

2.  **Product Integration**:
    - Update `PolySynth/Voice.h` to include `sea_oscillator.h` instead of `Oscillator.h`.
    - Instantiate `sea::Oscillator` (it will use `double` by default on Desktop).
    - Verify sound output is identical.
