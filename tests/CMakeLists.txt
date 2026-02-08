cmake_minimum_required(VERSION 3.14)
project(PolySynthTests)

set(CMAKE_CXX_STANDARD 17)

# Includes
include_directories(../src/core)
include_directories(.) # For catch.hpp if it's here, or we fetch it.

# Fetch Catch2 if not present (simulated for now, assumes single header in external or local)
# For this bootstrap, we will assume catch.hpp is in ../external/catch2/catch.hpp

# We need to make sure the external directory exists or we fetch it.
# Let's try to find it or download it.

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2/catch.hpp)
    message(STATUS "Downloading Catch2...")
    file(DOWNLOAD 
        https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/catch2/catch.hpp
    )
endif()

include_directories(../external/catch2)

# Source Files
set(SOURCES
    main_test.cpp
    Test_Engine_Basic.cpp
    Test_Oscillator.cpp
    Test_Engine_Audio.cpp
)

add_executable(run_tests ${SOURCES})
