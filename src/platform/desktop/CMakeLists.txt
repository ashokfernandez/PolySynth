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
    ${IPLUG2_DIR}/IGraphics
    ${IPLUG2_DIR}/IGraphics/Controls
    ${IPLUG2_DIR}/IGraphics/Drawing
    ${IPLUG2_DIR}/IPlug/Extras/WebView
    ${IPLUG2_DIR}/Dependencies/Extras/nlohmann
)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    PolySynth.cpp
    PolySynth.h
    PolySynth_DSP.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  LINK
    iPlug2::IGraphics::NanoVG
  NANOVG
)

# Apply Metal to all
target_compile_definitions(${PROJECT_NAME}-app PUBLIC IGRAPHICS_METAL)
target_compile_definitions(${PROJECT_NAME}-au PUBLIC IGRAPHICS_METAL)
target_compile_definitions(${PROJECT_NAME}-vst3 PUBLIC IGRAPHICS_METAL)
target_compile_definitions(PolySynthAU-framework PUBLIC IGRAPHICS_METAL)

# Function to remove definitions
function(remove_definitions targetName)
  if(TARGET ${targetName})
    get_target_property(defs ${targetName} COMPILE_DEFINITIONS)
    if(defs)
      list(REMOVE_ITEM defs "IGRAPHICS_GL2")
      list(REMOVE_ITEM defs "IPLUG_WEBVIEW")
      list(REMOVE_ITEM defs "WEBVIEW_EDITOR_DELEGATE")
      list(REMOVE_ITEM defs "WEBVIEW_EDITOR_DELEGATE=1")
      list(REMOVE_ITEM defs "NO_IGRAPHICS")
      set_target_properties(${targetName} PROPERTIES COMPILE_DEFINITIONS "${defs}")
      message(STATUS "Sanitized definitions for ${targetName}")
    endif()
  endif()
endfunction()

# Sanitize targets
remove_definitions(${PROJECT_NAME}-app)
remove_definitions(${PROJECT_NAME}-au)
remove_definitions(${PROJECT_NAME}-vst3)
remove_definitions(PolySynthAU-framework)
