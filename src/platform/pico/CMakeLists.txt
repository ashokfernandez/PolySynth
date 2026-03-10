cmake_minimum_required(VERSION 3.15)

# ── Pico SDK bootstrap (must come before project()) ──────────────────────
# Board selection: pico2_w (RP2350, default) or pico_w (RP2040, for Wokwi emulation)
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2_w)
endif()
if(NOT DEFINED PICO_PLATFORM)
    if(PICO_BOARD STREQUAL "pico2_w")
        set(PICO_PLATFORM rp2350-arm-s)
    else()
        # RP2040 — no PICO_PLATFORM override needed (SDK defaults to rp2040)
    endif()
endif()
include(pico_sdk_import.cmake)

project(polysynth_pico C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Initialize the Pico SDK (must come after project())
pico_sdk_init()

# ── Hard-float ABI ────────────────────────────────────────────────────────
# The Pico SDK defaults to -mfloat-abi=softfp, which passes float args via
# integer registers (requiring register-file transfers on every call).
# Hard-float passes floats directly in FPU registers, saving ~5-10% in
# function-heavy DSP code. Safe because SDK sources are compiled in-tree
# with the same flags.
string(REPLACE "-mfloat-abi=softfp" "-mfloat-abi=hard" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-mfloat-abi=softfp" "-mfloat-abi=hard" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-mfloat-abi=softfp" "-mfloat-abi=hard" CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")

# ── Embedded compile definitions ─────────────────────────────────────────
# These propagate to ALL targets in this build tree via SEA_DSP INTERFACE.
# POLYSYNTH_USE_FLOAT:       sample_t = float (not double)
# SEA_PLATFORM_EMBEDDED:     sea::Real = float, enables embedded codepaths
# POLYSYNTH_MAX_VOICES=4:    4 voices to fit in 520KB SRAM
set(SEA_PLATFORM_EMBEDDED ON CACHE BOOL "Embedded platform" FORCE)

# ── SEA_DSP library ──────────────────────────────────────────────────────
# Same pattern as /tests/CMakeLists.txt
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/SEA_DSP"
                 "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_DSP")

# ── SEA_Util library ─────────────────────────────────────────────────────
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/SEA_Util"
                 "${CMAKE_CURRENT_BINARY_DIR}/libs/SEA_Util")

# ── Firmware executable ──────────────────────────────────────────────────
add_executable(polysynth_pico
    main.cpp
    audio_i2s_driver.cpp
    command_parser.cpp
)

target_compile_definitions(polysynth_pico PRIVATE
    POLYSYNTH_USE_FLOAT
    POLYSYNTH_MAX_VOICES=4
    PICO_AUDIO_SAMPLE_RATE=48000
    PICO_AUDIO_BUFFER_FRAMES=256
    POLYSYNTH_DEPLOY_CHORUS=0
    POLYSYNTH_DEPLOY_DELAY=0
    POLYSYNTH_DEPLOY_LIMITER=0
    SEA_FAST_MATH
)

# Catch float-to-double promotions in hot paths (these cost ~10x on Cortex-M33)
target_compile_options(polysynth_pico PRIVATE
    -Wall -Wextra
    -Wdouble-promotion
    -fno-exceptions
    -fno-rtti
)

target_include_directories(polysynth_pico PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/core"
)

target_link_libraries(polysynth_pico PRIVATE
    pico_stdlib
    pico_cyw43_arch_none    # CYW43 for onboard LED (no WiFi stack)
    pico_multicore          # Dual-core: audio DSP on core 1
    hardware_pio
    hardware_dma
    hardware_clocks
    SEA_DSP
    SEA_Util
)

# Generate PIO header from .pio assembly
pico_generate_pio_header(polysynth_pico ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)

# USB serial output (not UART) — Pico 2 W connects via USB
pico_enable_stdio_usb(polysynth_pico 1)
pico_enable_stdio_uart(polysynth_pico 0)

# Generate .uf2 for drag-and-drop flashing
pico_add_extra_outputs(polysynth_pico)
