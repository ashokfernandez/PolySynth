cmake_minimum_required(VERSION 3.14)
project(PolySynth VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# ---------------------------------------------------------------------------
# Parse config.h as the single source of truth for plugin identity.
#
# Keeping these values here and in config.h in sync ensures that:
#   - CFBundleIdentifier in every Info.plist matches the BUNDLE_ID C macro
#   - [NSBundle bundleWithIdentifier:] can find the plugin at runtime
#   - Font loading (and anything else using LocateResource) works in AU/VST3
#
# To rename the plugin or company:
#   1. Edit config.h (PLUG_NAME, BUNDLE_NAME, BUNDLE_MFR, BUNDLE_DOMAIN, etc.)
#   2. Re-run cmake — the plists are regenerated automatically.
#   Nothing else needs to change.
# ---------------------------------------------------------------------------
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/config.h" _config_h)

# Extract a double-quoted string value:  #define KEY "value"
macro(_cfg_string out_var macro_name)
  if(_config_h MATCHES "#define[ \t]+${macro_name}[ \t]+\"([^\"]+)\"")
    set(${out_var} "${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "config.h: could not parse #define ${macro_name} \"...\"")
  endif()
endmacro()

# Extract a single-quoted 4CC:  #define KEY 'abcd'
macro(_cfg_4cc out_var macro_name)
  if(_config_h MATCHES "#define[ \t]+${macro_name}[ \t]+'([^']+)'")
    set(${out_var} "${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "config.h: could not parse #define ${macro_name} '....'")
  endif()
endmacro()

# Extract a hex literal:  #define KEY 0xNNNNNNNN
macro(_cfg_hex out_var macro_name)
  if(_config_h MATCHES "#define[ \t]+${macro_name}[ \t]+(0x[0-9A-Fa-f]+)")
    set(${out_var} "${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "config.h: could not parse #define ${macro_name} 0x...")
  endif()
endmacro()

_cfg_string(PLUG_DISPLAY_NAME   PLUG_NAME)
_cfg_string(PLUG_COPYRIGHT      PLUG_COPYRIGHT_STR)
_cfg_string(PLUG_VERSION_STRING PLUG_VERSION_STR)
_cfg_string(PLUG_BUNDLE_NAME    BUNDLE_NAME)
_cfg_string(PLUG_BUNDLE_MFR     BUNDLE_MFR)
_cfg_string(PLUG_BUNDLE_DOMAIN  BUNDLE_DOMAIN)
_cfg_4cc   (PLUG_UNIQUE_ID_4CC  PLUG_UNIQUE_ID)
_cfg_4cc   (PLUG_MFR_ID_4CC     PLUG_MFR_ID)
_cfg_hex   (PLUG_VERSION_HEX_STR PLUG_VERSION_HEX)

# Convert hex version to decimal integer for the AU plist <integer> field.
math(EXPR PLUG_VERSION_HEX_DEC "${PLUG_VERSION_HEX_STR}")

# Derived: the bundle identifier base shared by all formats.
# Matches what the BUNDLE_ID C macro expands to in IPlug_include_in_plug_hdr.h:
#   BUNDLE_DOMAIN "." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME
set(PLUG_BUNDLE_ID_BASE "${PLUG_BUNDLE_DOMAIN}.${PLUG_BUNDLE_MFR}")

message(STATUS "Plugin identity from config.h:")
message(STATUS "  Display name : ${PLUG_DISPLAY_NAME}")
message(STATUS "  Bundle ID    : ${PLUG_BUNDLE_ID_BASE}.{audiounit,vst3,app,...}.${PLUG_BUNDLE_NAME}")
message(STATUS "  Version      : ${PLUG_VERSION_STRING}")

# Generate plists from .in templates so they always match config.h.
# configure_file replaces @VAR@ tokens at cmake-configure time.
# iPlug2 cmake points PLUG_RESOURCES_DIR at resources/ by default — we generate
# the plists directly there so the path stays the same.
foreach(_fmt AU VST3 macOS AAX CLAP)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/${PROJECT_NAME}-${_fmt}-Info.plist.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/${PROJECT_NAME}-${_fmt}-Info.plist"
    @ONLY
  )
endforeach()

# Add core include directory
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/modulation
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../core/oscillator
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../UI/Controls
    ${IPLUG2_DIR}/Dependencies/Extras/nlohmann
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/SEA_DSP/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/SEA_Util/include
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
    resources/fonts/Roboto-Bold.ttf
)
