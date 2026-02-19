cmake_minimum_required(VERSION 3.14)
project(PolySynthTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Compile database (required by Cppcheck and Clang-Tidy for accurate analysis)
# ---------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Load project cmake modules
# (located at <repo_root>/cmake/ which is one level above tests/)
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(StaticAnalysis)
include(Sanitizers)

# ---------------------------------------------------------------------------
# Directories
# ---------------------------------------------------------------------------
include_directories(../src/core)
include_directories(../external/iPlug2/Dependencies/Extras/nlohmann)
include_directories(utils)
include_directories(mocks)

# ---------------------------------------------------------------------------
# Fetch Catch2 (single-header, v2)
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2/catch.hpp)
    message(STATUS "Downloading Catch2...")
    file(DOWNLOAD
        https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2/catch.hpp
    )
endif()
include_directories(../external/catch2)

# ---------------------------------------------------------------------------
# SEA_DSP Library
# ---------------------------------------------------------------------------
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../libs/SEA_DSP"
                 "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_DSP")

# ---------------------------------------------------------------------------
# SEA_Util Library
# ---------------------------------------------------------------------------
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../libs/SEA_Util"
                 "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_Util")

# ---------------------------------------------------------------------------
# Unit Tests target
# ---------------------------------------------------------------------------
set(UNIT_TEST_SOURCES
    main_test.cpp
    unit/Test_UI_Components.cpp
    unit/Test_Engine_Basic.cpp
    unit/Test_Engine_Audio.cpp
    unit/Test_VoiceManager_Poly.cpp
    unit/Test_SPSCRingBuffer.cpp
    unit/Test_VoiceStateMachine.cpp
    unit/Test_Effects.cpp
    unit/Test_PresetManager.cpp
    unit/Test_FactoryPresets.cpp
    unit/Test_PresetFileIO.cpp
    unit/Test_PolyMod.cpp
    unit/Test_SynthState.cpp
    unit/Test_VoiceAllocator.cpp
    unit/Test_VoiceAllocator.cpp
    unit/Test_VoiceAllocator_Regressions.cpp
    unit/Test_VoiceAllocation.cpp
    unit/Test_TheoryEngine.cpp
    unit/Test_Portamento.cpp
    unit/Test_StereoPanning.cpp
    unit/Test_VoiceManager_Features.cpp
    ../src/core/PresetManager.cpp
)

add_executable(run_tests ${UNIT_TEST_SOURCES})
target_link_libraries(run_tests PRIVATE SEA_DSP SEA_Util)

# Apply compiler warnings and runtime sanitizers to the unit test binary.
# Static analysis (Cppcheck / Clang-Tidy) is enabled only when the
# POLYSYNTH_ENABLE_STATIC_ANALYSIS option is ON to avoid slowing normal
# incremental builds. The CI lint job sets it explicitly.
polysynth_enable_compiler_warnings(run_tests)
polysynth_apply_sanitizers(run_tests)

option(POLYSYNTH_ENABLE_STATIC_ANALYSIS
    "Enable Cppcheck and Clang-Tidy during compilation" OFF)
if(POLYSYNTH_ENABLE_STATIC_ANALYSIS)
    polysynth_enable_cppcheck(run_tests)
    polysynth_enable_clang_tidy(run_tests)
endif()

# Sanitizer env vars are set directly in the CI yaml (and dev.sh) on the
# run_tests invocation line, so no ctest property needed here.

# ---------------------------------------------------------------------------
# Demo executables
# ---------------------------------------------------------------------------
set(ALL_DEMO_TARGETS
    demo_render_wav
    demo_adsr
    demo_engine_poly
    demo_filter_sweep
    demo_poly_chords
    demo_polymod
    demo_waveforms
    demo_pulse_width
    demo_osc_mix
    demo_filter_resonance
    demo_filter_envelope
    demo_prophet_filter
    demo_lfo_vibrato
    demo_lfo_tremolo
    demo_lfo_filter_wobble
    demo_factory_presets
    demo_fx_engine
)

add_executable(demo_render_wav      demos/demo_render_wav.cpp)
add_executable(demo_adsr            demos/demo_adsr.cpp)
add_executable(demo_engine_poly     demos/demo_engine_poly.cpp)
add_executable(demo_filter_sweep    demos/demo_filter_sweep.cpp)
add_executable(demo_poly_chords     demos/demo_poly_chords.cpp)
add_executable(demo_polymod         demos/demo_polymod.cpp)
add_executable(demo_waveforms       demos/demo_waveforms.cpp)
add_executable(demo_pulse_width     demos/demo_pulse_width.cpp)
add_executable(demo_osc_mix         demos/demo_osc_mix.cpp)
add_executable(demo_filter_resonance demos/demo_filter_resonance.cpp)
add_executable(demo_filter_envelope  demos/demo_filter_envelope.cpp)
add_executable(demo_prophet_filter   demos/demo_prophet_filter.cpp)
add_executable(demo_lfo_vibrato      demos/demo_lfo_vibrato.cpp)
add_executable(demo_lfo_tremolo      demos/demo_lfo_tremolo.cpp)
add_executable(demo_lfo_filter_wobble demos/demo_lfo_filter_wobble.cpp)
add_executable(demo_factory_presets  demos/demo_factory_presets.cpp)
add_executable(demo_fx_engine        demos/demo_fx_engine.cpp)

foreach(_demo IN LISTS ALL_DEMO_TARGETS)
    target_link_libraries(${_demo} PRIVATE SEA_DSP SEA_Util)
    polysynth_enable_compiler_warnings(${_demo})
    polysynth_apply_sanitizers(${_demo})
endforeach()

# ---------------------------------------------------------------------------
# Sanitizer summary (printed at configure time)
# ---------------------------------------------------------------------------
polysynth_sanitizer_summary()
