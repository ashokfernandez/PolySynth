cmake_minimum_required(VERSION 3.15)
project(SEA_DSP)

add_library(SEA_DSP INTERFACE)

target_include_directories(SEA_DSP INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

option(SEA_PLATFORM_EMBEDDED "Build for embedded platform (float)" OFF)

if(SEA_PLATFORM_EMBEDDED)
    target_compile_definitions(SEA_DSP INTERFACE SEA_PLATFORM_EMBEDDED)
endif()

# Tests
option(SEA_BUILD_TESTS "Build SEA_DSP unit tests" OFF)
if(SEA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
