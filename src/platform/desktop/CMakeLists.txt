cmake_minimum_required(VERSION 3.14)
project(PolySynth VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

include_directories(${IPLUG2_DIR}/IPlug/Extras/WebView)
include_directories(${IPLUG2_DIR}/Dependencies/IGraphics/MetalNanoVG/src)
add_compile_definitions(WEBVIEW_EDITOR_DELEGATE=1)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    PolySynth.cpp
    PolySynth.h
    PolySynth_DSP.h
    resources/resource.h
  WEB_RESOURCES
    resources/web/index.html
    resources/web/style.css
    resources/web/main.js
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  LINK
    iPlug2::Extras::Synth
  EXTRA_INC_PATHS
    ${IPLUG2_DIR}/IPlug/Extras/WebView
  UI WEBVIEW
)
