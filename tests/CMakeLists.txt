cmake_minimum_required(VERSION 3.14)
project(PolySynthTests)

set(CMAKE_CXX_STANDARD 17)

# Directories
include_directories(../src/core)
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
    unit/Test_Engine_Audio.cpp
    unit/Test_ADSREnvelope.cpp
)

add_executable(run_tests ${UNIT_TEST_SOURCES})

# --- Demos ---
# Refactored demo_render_wav to use WavWriter
add_executable(demo_render_wav demos/demo_render_wav.cpp)
add_executable(demo_adsr demos/demo_adsr.cpp)
add_executable(demo_engine_poly demos/demo_engine_poly.cpp)

# Future Demos
# add_executable(demo_adsr demos/demo_adsr.cpp)
