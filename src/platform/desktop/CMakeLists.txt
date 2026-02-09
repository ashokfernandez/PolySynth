cmake_minimum_required(VERSION 3.14)
project(PolySynth VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Include WebView module
include(${IPLUG2_DIR}/Scripts/cmake/WebView.cmake)

# Add core include directory
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/modulation
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/oscillator
    ${IPLUG2_DIR}/IPlug/Extras/WebView
    ${IPLUG2_DIR}/Dependencies/Extras/nlohmann
)

# Glob web resources from the dist folder (Vite output)
file(GLOB WEB_RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/web/dist/index.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/web/dist/assets/*"
)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    PolySynth.cpp
    PolySynth.h
    PolySynth_DSP.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
    ${WEB_RESOURCES}
  LINK
    iPlug2::WebView
)
