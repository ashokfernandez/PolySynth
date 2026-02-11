# SEA_DSP

**Side Effects Audio DSP Library** — A portable C++ library for real-time audio digital signal processing.

## What is SEA_DSP?

SEA_DSP is a collection of high-quality DSP algorithms designed for synthesizers, effects processors, and other real-time audio applications. The library emphasizes correctness, portability, and performance, with support for both desktop (64-bit floating point) and embedded (32-bit floating point) targets.

### Key Features

- **Lightweight Integration**: Core DSP algorithms with minimal dependencies — can be header-only when using default backends
- **Dual-Precision**: Compile-time selection between `double` (desktop) and `float` (embedded) precision
- **Template-Based Filters**: Type-safe filter implementations work with any floating-point type
- **Real-Time Safe**: No allocations in audio processing paths (with noted exceptions)
- **Extensively Tested**: 206+ assertions across 30 test cases verify correctness on both platforms
- **Backend Flexibility**: Optional DaisySP integration for band-limited oscillators (requires linking DaisySP library)

## What's Inside

SEA_DSP provides three categories of DSP components:

### 1. Virtual Analog Filters

High-quality filters using topology-preserving transform (TPT) and other proven techniques:

- **TPT Integrator** (`sea_tpt_integrator.h`) — Building block for VA filters, implements Zavalishin's TPT equations
- **State Variable Filter** (`sea_svf.h`) — 12dB/octave multi-mode filter (LP/BP/HP simultaneous outputs)
- **Ladder Filter** (`sea_ladder_filter.h`) — Classic 4-pole 24dB/octave lowpass with transistor/diode models
- **Cascade Filter** (`sea_cascade_filter.h`) — Cascaded SVF stages with 12/24dB slope options
- **Classical Filter** (`sea_classical_filter.h`) — Butterworth/Chebyshev designs using cascaded biquads

### 2. Classical Filters

Standard IIR filter designs:

- **Biquad Filter** (`sea_biquad_filter.h`) — RBJ Audio EQ Cookbook implementations (LP/HP/BP/Notch)
- **Sallen-Key Filter** (`sea_sk_filter.h`) — Classic analog filter topology

### 3. Modulation & Oscillation

Essential synthesis building blocks:

- **Oscillator** (`sea_oscillator.h`) — Basic waveforms (saw, square, triangle, sine) with optional DaisySP band-limited backend
- **ADSR Envelope** (`sea_adsr.h`) — Linear ADSR with optimized release calculation
- **LFO** (`sea_lfo.h`) — Low-frequency oscillator for modulation

### 4. Platform Utilities

- **Platform Abstraction** (`sea_platform.h`) — Precision selection, constants, inline hints
- **Math Facade** (`sea_math.h`) — Unified math operations with fast-math hooks for embedded targets

## Design Philosophy

### Correctness First

Every DSP component is verified against reference implementations through bit-exact testing. We prioritize algorithmic correctness over micro-optimizations — the compiler is surprisingly good at optimization when given clean code.

### Type Safety

Templated filters (`template<typename T>`) allow mixing float and double precision in the same binary. Constants are properly cast to template parameter types, preventing subtle precision bugs. Non-templated components use `sea::Real`, which is compile-time configured based on platform.

### Real-Time Constraints

Audio processing methods avoid:
- Dynamic memory allocation (with one documented exception: `ClassicalFilter` uses `std::vector` for stage storage)
- System calls or I/O operations
- Unbounded loops or recursion
- Division in hot paths where multiplication can be pre-computed

### Incremental Evolution

The library follows a "port first, optimize later" philosophy. New algorithms are first implemented in a straightforward, verifiable form. Optimizations come after correctness is proven through tests.

## Platform Support

### Desktop Targets

Default configuration uses `double` precision throughout:

```cpp
// On desktop, sea::Real = double
sea::BiquadFilter<double> filter;  // Explicit type
sea::Oscillator osc;                // Uses sea::Real (double)
```

### Embedded Targets

Set `SEA_DSP_TARGET_PLATFORM=daisy_mcu` to enable `float` precision:

```cpp
// On embedded, sea::Real = float
sea::BiquadFilter<float> filter;   // Explicit type
sea::Oscillator osc;                // Uses sea::Real (float)
```

This affects:
- `sea::Real` type definition
- Math constants (`kPi`, `kTwoPi`)
- All non-templated components (Oscillator, ADSR, LFO)

### Mixed-Precision Builds

Templated filters can instantiate both types in a single binary:

```cpp
sea::SVFilter<double> highPrecisionFilter;  // For mastering
sea::SVFilter<float> realtimeFilter;        // For low-latency DSP
```

## Backend Configuration

SEA_DSP provides two implementation paths for basic waveform generators:

### SEA Core Backend (Default)

Native implementations optimized for simplicity and portability:
- Naive oscillators (aliasing waveforms)
- Linear ADSR envelopes
- Simple LFO shapes

**Minimal dependencies. Can be header-only for filter-only usage.**

### DaisySP Backend (Optional)

High-quality implementations from the Electrosmith DaisySP library:
- Band-limited oscillators (anti-aliased)
- Optimized envelope generators
- Enhanced modulation sources

**Requires linking against DaisySP library.**

Configure per-component:

```bash
cmake -DSEA_DSP_OSC_BACKEND=daisysp \
      -DSEA_DSP_ADSR_BACKEND=sea_core \
      -DSEA_DSP_LFO_BACKEND=daisysp
```

**Note**: DaisySP must be available in `external/daisysp` when using DaisySP backends.

## Dependencies

### Core Library (SEA Core Backend)

**Minimal dependencies** when using default backends:
- **C++17 compiler** (tested with GCC 9+, Clang 10+, MSVC 2019+)
- **CMake 3.15+** (for integration)
- **Standard Library** (`<cmath>`, `<algorithm>`, `<vector>`)

### Optional Dependencies

- **DaisySP** (optional) — For band-limited oscillators and enhanced components
  - License: MIT
  - Source: https://github.com/electro-smith/DaisySP
  - Auto-detected at `external/daisysp/` relative to library root

### Test Suite Dependencies

- **Catch2 v2.13.9** (single-header) — Automatically downloaded if not found
  - License: BSL-1.0
  - Fallback location: `external/catch2/catch.hpp`
  - Auto-download to build tree if missing

## Building and Testing

### Standalone Build

Build the library tests in isolation:

```bash
# Desktop (double precision)
cmake -S . -B build -DSEA_BUILD_TESTS=ON
cmake --build build
./build/tests/sea_tests
```

### Embedded Target

```bash
# Embedded (float precision)
cmake -S . -B build_embedded \
  -DSEA_BUILD_TESTS=ON \
  -DSEA_DSP_TARGET_PLATFORM=daisy_mcu
cmake --build build_embedded
./build_embedded/tests/sea_tests
```

### With DaisySP Backend

```bash
# Ensure DaisySP is available
./scripts/download_dependencies.sh  # If using a parent project

cmake -S . -B build_daisysp \
  -DSEA_BUILD_TESTS=ON \
  -DSEA_DSP_OSC_BACKEND=daisysp \
  -DSEA_DSP_ADSR_BACKEND=daisysp \
  -DSEA_DSP_LFO_BACKEND=daisysp
cmake --build build_daisysp
./build_daisysp/tests/sea_tests
```

### Integration Into Your Project

#### CMake Subdirectory

```cmake
add_subdirectory(path/to/SEA_DSP)
target_link_libraries(your_target PRIVATE SEA_DSP)

# Optional: Configure platform
set(SEA_DSP_TARGET_PLATFORM "daisy_mcu" CACHE STRING "" FORCE)

# Optional: Configure backends
set(SEA_DSP_OSC_BACKEND "daisysp" CACHE STRING "" FORCE)
```

#### Manual Include

```cpp
// Add include/sea_dsp/ to your include path
#include <sea_dsp/sea_biquad_filter.h>
#include <sea_dsp/sea_oscillator.h>

// Use the library
sea::BiquadFilter<double> lpf;
lpf.Init(48000.0);
lpf.SetParams(sea::FilterType::LowPass, 1000.0, 0.707);
double output = lpf.Process(input);
```

## Testing and Verification

### Test Coverage

The library includes **30 test cases** with **206 assertions** covering:

**Self-Contained Headers** (individual test files per filter):
- Each filter header tested in isolation to verify it brings all dependencies
- BiquadFilter, TPTIntegrator, LadderFilter, ClassicalFilter, CascadeFilter, SKFilter
- Prevents transitive dependency masking where includes can hide missing dependencies

**Type Safety** (individual test files):
- Float vs double consistency for all templated filters
- Math constant casting correctness (`Test_MathConstants.cpp`)
- Long-running precision tests (1000+ samples)

**Filter Behavior** (`Test_Filters.cpp`):
- TPT integrator gain calculation and stability
- State variable filter response (LP/BP/HP)
- Biquad filter DC/Nyquist response and stability
- Classical filter include dependencies

**Modulation** (`Test_ADSR.cpp`, `Test_ADSR_Performance.cpp`, `Test_LFO.cpp`):
- ADSR stage transitions and envelope shapes
- Release correctness from different trigger levels
- Performance optimization verification (no division in NoteOff)
- LFO waveform accuracy

**Platform Compatibility** (`Test_Platform.cpp`, `Test_Math.cpp`):
- Type definitions match expected precision
- Math wrappers produce correct results
- Constants have correct values

**Oscillators** (`Test_Oscillator.cpp`):
- Waveform generation accuracy
- Frequency calculation precision
- Phase wrap boundary conditions (platform-aware)

### Verification Methodology

**Bit-Exact Testing**: Core DSP algorithms are verified against reference implementations with sub-epsilon tolerance (<1e-14 for double).

**Dual-Precision Testing**: All templated components are instantiated and tested with both `float` and `double` types to catch precision-dependent bugs.

**Platform Testing**: Tests run on both desktop (double) and embedded (float) configurations in continuous integration.

**Regression Protection**: Test suite runs on every commit, catching behavioral changes immediately.

### Running Tests

```bash
# All tests
./sea_tests

# Specific test suites
./sea_tests "[TypeSafety]"      # Type safety verification
./sea_tests "[ADSR]"            # All ADSR tests
./sea_tests "[Filter]"          # All filter tests
./sea_tests "[.benchmark]"      # Performance benchmarks only

# With detailed output
./sea_tests -s                  # Show all assertions
```

## Performance Characteristics

### Computational Complexity

| Component | Per-Sample Cost | Notes |
|-----------|----------------|-------|
| BiquadFilter | ~10 FLOPs | Direct Form II, 2 states |
| SVFilter | ~15 FLOPs | Two TPT integrators |
| LadderFilter | ~40 FLOPs | Four TPT integrators + tanh |
| Oscillator (sea_core) | ~5 FLOPs | Naive waveforms |
| Oscillator (daisysp) | ~20 FLOPs | Band-limited, anti-aliased |
| ADSREnvelope | ~3 FLOPs | Linear segments |

### Optimization Highlights

**ADSR NoteOff Optimization**: Pre-calculates `1/(R*SR)` in parameter updates, eliminating division in the note-off path. Measured improvement: ~5-10x faster on ARM Cortex-M7.

**TPT Filters**: Use topology-preserving transforms to minimize per-sample computation while maintaining filter characteristics.

**Inline Hints**: Hot-path methods marked with `SEA_INLINE` for aggressive inlining across compilers.

## API Stability

SEA_DSP is currently in active development. While we maintain backward compatibility within major versions:

- **Public APIs**: Method signatures and class names are stable
- **Behavioral Changes**: DSP algorithm improvements may alter output (noted in changelog)
- **CMake Options**: New options may be added, existing options won't be removed without deprecation

We follow semantic versioning once we reach v1.0.0.

## Known Limitations

### ClassicalFilter Memory Allocation

`sea_classical_filter.h` uses `std::vector` for stage storage, allocating during `SetConfig()`. This is **not real-time safe**. Workaround: Pre-configure filters during initialization, not in audio callback.

Future: Will provide fixed-size array alternative with compile-time maximum order.

### SEA_FAST_MATH Not Enabled

Fast approximations for `tanh()` and other transcendentals exist but are disabled pending thorough error analysis. Use standard library implementations for now.

### Naive Oscillators (sea_core)

Default oscillator backend produces aliasing artifacts. For production audio, use DaisySP backend or implement your own band-limited oscillators.

## Contributing

SEA_DSP welcomes contributions! When adding DSP algorithms:

1. **Port faithfully first**: Implement the algorithm straightforwardly from reference (paper, existing code)
2. **Add bit-exact tests**: Verify against known-good output with tight tolerances
3. **Document algorithm**: Reference papers, explain parameter ranges, note limitations
4. **Benchmark if optimizing**: Show performance improvement, verify correctness maintained

See `CONTRIBUTING.md` for detailed guidelines (TODO).

## License

SEA_DSP is released under the MIT License. See `LICENSE` file for details.

### Third-Party Code

- **DaisySP** (optional dependency): MIT License — Copyright Electrosmith, LLC
- **Catch2** (test-only dependency): BSL-1.0 License — Copyright Two Blue Cubes Ltd.

## Acknowledgments

Filter designs based on:
- Vadim Zavalishin's "The Art of VA Filter Design" (TPT integrator, SVF, ladder)
- Robert Bristow-Johnson's Audio EQ Cookbook (biquad coefficients)
- Analyzing classic analog synthesizer topologies and published schematics

The "port first, optimize later" philosophy follows wisdom from:
- Donald Knuth: "Premature optimization is the root of all evil"
- The Audio Programming community's emphasis on correctness

## Support and Community

- **Issues**: Report bugs and request features via GitHub Issues
- **Discussions**: Ask questions and share projects in GitHub Discussions
- **Documentation**: Full API reference at `docs/` (TODO)

## Roadmap

### Short-Term (Current)

- ✅ Core filter library with template support
- ✅ Dual-precision testing infrastructure
- ✅ DaisySP backend integration
- 🔄 Eliminate ClassicalFilter allocations
- 🔄 Fast-math approximations with error bounds

### Mid-Term (v1.0)

- [ ] Comprehensive API documentation
- [ ] Example projects and tutorials
- [ ] Performance profiling suite
- [ ] SIMD-optimized variants for critical paths
- [ ] Additional filter types (comb, allpass, formant)

### Long-Term (v2.0+)

- [ ] JUCE module integration
- [ ] Python bindings for prototyping
- [ ] WebAssembly build target
- [ ] GPU-accelerated variants (CUDA/Metal)

## Version History

See `CHANGELOG.md` for detailed version history.

**Current**: v0.2.0 (2024-02) — Type safety fixes, ADSR optimization, comprehensive test suite

---

**Made with ❤️ for the audio programming community**
