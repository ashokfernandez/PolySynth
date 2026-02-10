cmake_minimum_required(VERSION 3.15)
project(SEA_DSP)

add_library(SEA_DSP INTERFACE)

target_include_directories(SEA_DSP INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

option(SEA_PLATFORM_EMBEDDED "Build for embedded platform (float)" OFF)
set(SEA_DSP_TARGET_PLATFORM "desktop" CACHE STRING "SEA_DSP target platform (desktop or daisy_mcu)")
set_property(CACHE SEA_DSP_TARGET_PLATFORM PROPERTY STRINGS desktop daisy_mcu)
set(SEA_DSP_OSC_BACKEND "sea_core" CACHE STRING "Oscillator backend (sea_core or daisysp)")
set_property(CACHE SEA_DSP_OSC_BACKEND PROPERTY STRINGS sea_core daisysp)
set(SEA_DSP_ADSR_BACKEND "sea_core" CACHE STRING "ADSR backend (sea_core or daisysp)")
set_property(CACHE SEA_DSP_ADSR_BACKEND PROPERTY STRINGS sea_core daisysp)
set(SEA_DSP_LFO_BACKEND "sea_core" CACHE STRING "LFO backend (sea_core or daisysp)")
set_property(CACHE SEA_DSP_LFO_BACKEND PROPERTY STRINGS sea_core daisysp)

# Backward-compatibility shim for earlier SEA_DSP_TARGET naming.
if(DEFINED SEA_DSP_TARGET AND NOT DEFINED SEA_DSP_TARGET_PLATFORM)
    set(SEA_DSP_TARGET_PLATFORM "${SEA_DSP_TARGET}" CACHE STRING "SEA_DSP target platform" FORCE)
endif()
if(SEA_DSP_TARGET_PLATFORM STREQUAL "armv7")
    set(SEA_DSP_TARGET_PLATFORM "daisy_mcu" CACHE STRING "SEA_DSP target platform" FORCE)
endif()

if(NOT SEA_DSP_TARGET_PLATFORM STREQUAL "desktop" AND NOT SEA_DSP_TARGET_PLATFORM STREQUAL "daisy_mcu")
    message(FATAL_ERROR "Unknown SEA_DSP_TARGET_PLATFORM='${SEA_DSP_TARGET_PLATFORM}'. Use desktop or daisy_mcu.")
endif()

if(SEA_DSP_TARGET_PLATFORM STREQUAL "daisy_mcu")
    set(SEA_PLATFORM_EMBEDDED ON)
endif()

if(SEA_PLATFORM_EMBEDDED)
    target_compile_definitions(SEA_DSP INTERFACE SEA_PLATFORM_EMBEDDED)
endif()

function(_sea_dsp_define_component_backend component value)
    string(TOUPPER "${component}" component_upper)
    if(value STREQUAL "sea_core")
        target_compile_definitions(SEA_DSP INTERFACE "SEA_DSP_${component_upper}_BACKEND_SEA_CORE")
    elseif(value STREQUAL "daisysp")
        target_compile_definitions(SEA_DSP INTERFACE "SEA_DSP_${component_upper}_BACKEND_DAISYSP")
        set(SEA_DSP_NEEDS_DAISYSP ON PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Unknown backend '${value}' for ${component}. Use sea_core or daisysp.")
    endif()
endfunction()

set(SEA_DSP_NEEDS_DAISYSP OFF)
_sea_dsp_define_component_backend("osc" "${SEA_DSP_OSC_BACKEND}")
_sea_dsp_define_component_backend("adsr" "${SEA_DSP_ADSR_BACKEND}")
_sea_dsp_define_component_backend("lfo" "${SEA_DSP_LFO_BACKEND}")

if(SEA_DSP_NEEDS_DAISYSP)
    set(SEA_DSP_DAISYSP_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../../external/daisysp"
        CACHE PATH "Path to DaisySP source tree")

    if(NOT EXISTS "${SEA_DSP_DAISYSP_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "A DaisySP backend is enabled but DaisySP was not found at ${SEA_DSP_DAISYSP_DIR}. "
            "Run scripts/download_dependencies.sh first.")
    endif()

    if(NOT TARGET DaisySP)
        add_subdirectory("${SEA_DSP_DAISYSP_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/daisysp")
    endif()
    # Some DaisySP revisions do not reliably propagate C++ standard settings.
    set_property(TARGET DaisySP PROPERTY CXX_STANDARD 14)
    set_property(TARGET DaisySP PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET DaisySP PROPERTY CXX_EXTENSIONS OFF)
    target_link_libraries(SEA_DSP INTERFACE DaisySP)
endif()

# Tests
option(SEA_BUILD_TESTS "Build SEA_DSP unit tests" OFF)
if(SEA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
