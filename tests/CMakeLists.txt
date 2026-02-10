cmake_minimum_required(VERSION 3.14)
project(PolySynthTests)

set(CMAKE_CXX_STANDARD 17)

# Directories
include_directories(../src/core)
include_directories(../external/iPlug2/Dependencies/Extras/nlohmann)
include_directories(utils)

# Fetch Catch2
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2/catch.hpp)
    message(STATUS "Downloading Catch2...")
    file(DOWNLOAD 
        https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2/catch.hpp
    )
endif()
include_directories(../external/catch2)

# --- Unit Tests ---
set(UNIT_TEST_SOURCES
    main_test.cpp
    unit/Test_Engine_Basic.cpp
    unit/Test_Oscillator.cpp
    unit/Test_Oscillator_Waveforms.cpp
    unit/Test_Engine_Audio.cpp
    unit/Test_ADSREnvelope.cpp
    unit/Test_BiquadFilter.cpp
    unit/Test_LFO.cpp
    unit/Test_VoiceManager_Poly.cpp
    unit/Test_VAFilters.cpp
    unit/Test_FXEngine.cpp
    unit/Test_PresetManager.cpp
    unit/Test_FactoryPresets.cpp
    unit/Test_PresetFileIO.cpp
    ../src/core/PresetManager.cpp
)

add_executable(run_tests ${UNIT_TEST_SOURCES})

# --- Demos ---
add_executable(demo_render_wav demos/demo_render_wav.cpp)
add_executable(demo_adsr demos/demo_adsr.cpp)
add_executable(demo_engine_poly demos/demo_engine_poly.cpp)
add_executable(demo_filter_sweep demos/demo_filter_sweep.cpp)
add_executable(demo_poly_chords demos/demo_poly_chords.cpp)

# Oscillator demos
add_executable(demo_waveforms demos/demo_waveforms.cpp)
add_executable(demo_pulse_width demos/demo_pulse_width.cpp)
add_executable(demo_osc_mix demos/demo_osc_mix.cpp)

# Filter demos
add_executable(demo_filter_resonance demos/demo_filter_resonance.cpp)
add_executable(demo_filter_envelope demos/demo_filter_envelope.cpp)

# LFO demos
add_executable(demo_lfo_vibrato demos/demo_lfo_vibrato.cpp)
add_executable(demo_lfo_tremolo demos/demo_lfo_tremolo.cpp)
add_executable(demo_lfo_filter_wobble demos/demo_lfo_filter_wobble.cpp)

# Preset demo
add_executable(demo_factory_presets demos/demo_factory_presets.cpp)

# FX demo
add_executable(demo_fx_engine demos/demo_fx_engine.cpp)
