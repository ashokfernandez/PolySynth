cmake_minimum_required(VERSION 3.14)
project(PolySynth VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Add core include directory
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/modulation
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/oscillator
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../UI/Controls
    ${IPLUG2_DIR}/Dependencies/Extras/nlohmann
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/SEA_DSP/include
)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    PolySynth.cpp
    DemoSequencer.cpp
    PolySynth.h
    PolySynth_DSP.h
    resources/resource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/PresetManager.cpp
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
)
