cmake_minimum_required(VERSION 3.15)
project(SEA_DSP_Tests)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories("../include")

# Catch2 single-header provisioning for standalone test runs.
# Prefer a repository-local header if present, otherwise download to the build tree.
set(SEA_DSP_CATCH2_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/catch2/catch.hpp"
    CACHE FILEPATH "Path to Catch2 single-header file")

if(NOT EXISTS "${SEA_DSP_CATCH2_HEADER}")
    set(_sea_dsp_catch2_dir "${CMAKE_CURRENT_BINARY_DIR}/_deps/catch2")
    file(MAKE_DIRECTORY "${_sea_dsp_catch2_dir}")
    set(_sea_dsp_catch2_header "${_sea_dsp_catch2_dir}/catch.hpp")

    message(STATUS "Catch2 header not found at ${SEA_DSP_CATCH2_HEADER}")
    message(STATUS "Downloading Catch2 v2.13.9 to ${_sea_dsp_catch2_header}")
    file(DOWNLOAD
        "https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp"
        "${_sea_dsp_catch2_header}"
        SHOW_PROGRESS
        STATUS _sea_dsp_catch2_status
    )
    list(GET _sea_dsp_catch2_status 0 _sea_dsp_catch2_status_code)
    if(NOT _sea_dsp_catch2_status_code EQUAL 0)
        list(GET _sea_dsp_catch2_status 1 _sea_dsp_catch2_status_message)
        message(FATAL_ERROR
            "Failed to download Catch2 header: ${_sea_dsp_catch2_status_message}")
    endif()
    set(SEA_DSP_CATCH2_HEADER "${_sea_dsp_catch2_header}")
endif()

get_filename_component(SEA_DSP_CATCH2_INCLUDE_DIR "${SEA_DSP_CATCH2_HEADER}" DIRECTORY)
include_directories("${SEA_DSP_CATCH2_INCLUDE_DIR}")

# Source files
set(TEST_SOURCES
    main.cpp
    Test_Math.cpp
    Test_Platform.cpp
    Test_Oscillator.cpp
    Test_Filters.cpp
    Test_ADSR.cpp
    Test_LFO.cpp
    Test_TypeSafety.cpp
    Test_ADSR_Performance.cpp
)

add_executable(sea_tests ${TEST_SOURCES})

# Link against the library
target_link_libraries(sea_tests PRIVATE SEA_DSP)

# Add compilation definitions if needed
if(SEA_PLATFORM_EMBEDDED)
    target_compile_definitions(sea_tests PRIVATE SEA_PLATFORM_EMBEDDED)
endif()
