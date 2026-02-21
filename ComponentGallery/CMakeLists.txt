cmake_minimum_required(VERSION 3.14)
project(PolySynthGallery VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

set(IGRAPHICS_BACKEND "NANOVG" CACHE STRING "IGraphics drawing backend" FORCE)

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# UI controls live at repo root UI/Controls/
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../UI/Controls
    ${IPLUG2_DIR}/Dependencies/Extras/nlohmann
)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    ComponentGallery.cpp
    ComponentGallery.h
    config.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
    resources/fonts/Roboto-Bold.ttf
  FORMATS APP
)
