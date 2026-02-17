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

# --- SEA_DSP Library ---
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../libs/SEA_DSP" "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_DSP")

# --- SEA_Util Library ---
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../libs/SEA_Util" "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_Util")

# --- Unit Tests ---
set(UNIT_TEST_SOURCES
    main_test.cpp
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
    ../src/core/PresetManager.cpp
)

add_executable(run_tests ${UNIT_TEST_SOURCES})

# --- Demos ---
add_executable(demo_render_wav demos/demo_render_wav.cpp)
add_executable(demo_adsr demos/demo_adsr.cpp)
add_executable(demo_engine_poly demos/demo_engine_poly.cpp)
add_executable(demo_filter_sweep demos/demo_filter_sweep.cpp)
add_executable(demo_poly_chords demos/demo_poly_chords.cpp)
add_executable(demo_polymod demos/demo_polymod.cpp)

# Oscillator demos
add_executable(demo_waveforms demos/demo_waveforms.cpp)
add_executable(demo_pulse_width demos/demo_pulse_width.cpp)
add_executable(demo_osc_mix demos/demo_osc_mix.cpp)

# Filter demos
add_executable(demo_filter_resonance demos/demo_filter_resonance.cpp)
add_executable(demo_filter_envelope demos/demo_filter_envelope.cpp)
add_executable(demo_prophet_filter demos/demo_prophet_filter.cpp)

# LFO demos
add_executable(demo_lfo_vibrato demos/demo_lfo_vibrato.cpp)
add_executable(demo_lfo_tremolo demos/demo_lfo_tremolo.cpp)
add_executable(demo_lfo_filter_wobble demos/demo_lfo_filter_wobble.cpp)

# Preset demo
add_executable(demo_factory_presets demos/demo_factory_presets.cpp)

# FX demo
add_executable(demo_fx_engine demos/demo_fx_engine.cpp)

# Link SEA_DSP to all targets
target_link_libraries(run_tests PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_render_wav PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_adsr PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_engine_poly PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_filter_sweep PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_poly_chords PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_polymod PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_waveforms PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_pulse_width PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_osc_mix PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_filter_resonance PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_filter_envelope PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_prophet_filter PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_lfo_vibrato PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_lfo_tremolo PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_lfo_filter_wobble PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_factory_presets PRIVATE SEA_DSP SEA_Util)
target_link_libraries(demo_fx_engine PRIVATE SEA_DSP SEA_Util)
