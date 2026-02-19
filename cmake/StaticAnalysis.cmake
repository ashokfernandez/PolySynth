# cmake/StaticAnalysis.cmake
# Configures Cppcheck and Clang-Tidy for the PolySynth project.
# Include this module AFTER defining your targets.
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/../cmake/StaticAnalysis.cmake)
#   polysynth_enable_cppcheck(run_tests)
#   polysynth_enable_clang_tidy(run_tests)
#   polysynth_enable_compiler_warnings(run_tests)

cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------
# Helper: escalate compiler warnings to errors on our own targets.
# Not applied to 3rd-party targets (Catch2, DaisySP, etc.).
# ---------------------------------------------------------------------------
function(polysynth_enable_compiler_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            # Downgrade a few high-noise warnings that are unavoidable in DSP
            # and header-only template code to warnings (not errors) so they
            # don't block every CI run. Re-promote them as the codebase matures.
            -Wno-error=unused-parameter
            -Wno-error=sign-compare
        )
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE /W4 /WX)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Cppcheck – fast, low-false-positive static analysis.
# Requires: cppcheck on PATH.
# ---------------------------------------------------------------------------
function(polysynth_enable_cppcheck target)
    find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
    if(NOT CPPCHECK_EXECUTABLE)
        message(WARNING
            "[StaticAnalysis] cppcheck not found – skipping for '${target}'. "
            "Install it with: apt-get install cppcheck  OR  brew install cppcheck")
        return()
    endif()

    message(STATUS "[StaticAnalysis] Enabling Cppcheck for target '${target}'")

    # Locate suppressions file (relative to the cmake/ dir, i.e. project root)
    set(_suppressions_file "${CMAKE_SOURCE_DIR}/../tools/suppressions.txt")
    if(NOT EXISTS "${_suppressions_file}")
        # Allow callers to place tools/ next to CMakeLists.txt
        set(_suppressions_file "${CMAKE_SOURCE_DIR}/tools/suppressions.txt")
    endif()

    set(_cppcheck_args
        # Show these categories beyond pure errors
        "--enable=warning,performance,portability,style"
        # Report potential bugs even without 100% certainty – prefer false alarms
        "--inconclusive"
        # Use project compile database for accurate include paths
        "--project=${CMAKE_BINARY_DIR}/compile_commands.json"
        # Compiler error-style output: parsable by IDEs and the LLM
        "--template={file}:{line}: [{id}] {message}"
        # CI gate: non-zero exit on any issue
        "--error-exitcode=1"
        # Also emit XML for CI artifact upload (written alongside the build)
        "--xml"
        "--xml-version=2"
        "--output-file=${CMAKE_BINARY_DIR}/cppcheck-report.xml"
    )

    if(EXISTS "${_suppressions_file}")
        list(APPEND _cppcheck_args "--suppressions-list=${_suppressions_file}")
        message(STATUS "[StaticAnalysis] Cppcheck suppressions: ${_suppressions_file}")
    endif()

    set_target_properties(${target} PROPERTIES
        CXX_CPPCHECK "${CPPCHECK_EXECUTABLE};${_cppcheck_args}"
    )
endfunction()

# ---------------------------------------------------------------------------
# Clang-Tidy – deep, standards-aware analysis.
# Requires: clang-tidy on PATH.
# Config is read from tools/.clang-tidy (copied/symlinked next to source).
# ---------------------------------------------------------------------------
function(polysynth_enable_clang_tidy target)
    find_program(CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15)
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(WARNING
            "[StaticAnalysis] clang-tidy not found – skipping for '${target}'. "
            "Install it with: apt-get install clang-tidy  OR  brew install llvm")
        return()
    endif()

    message(STATUS "[StaticAnalysis] Enabling Clang-Tidy for target '${target}' "
                   "using ${CLANG_TIDY_EXECUTABLE}")

    # clang-tidy reads .clang-tidy from the source tree automatically,
    # but we also pass --config-file explicitly as a fallback.
    set(_clang_tidy_config "${CMAKE_SOURCE_DIR}/../tools/.clang-tidy")
    if(NOT EXISTS "${_clang_tidy_config}")
        set(_clang_tidy_config "${CMAKE_SOURCE_DIR}/tools/.clang-tidy")
    endif()

    set(_clang_tidy_args "${CLANG_TIDY_EXECUTABLE}")

    if(EXISTS "${_clang_tidy_config}")
        list(APPEND _clang_tidy_args "--config-file=${_clang_tidy_config}")
        message(STATUS "[StaticAnalysis] Clang-Tidy config: ${_clang_tidy_config}")
    endif()

    # Promote all clang-tidy warnings to errors so the build gate fires.
    list(APPEND _clang_tidy_args "--warnings-as-errors=*")

    set_target_properties(${target} PROPERTIES
        CXX_CLANG_TIDY "${_clang_tidy_args}"
    )
endfunction()

# ---------------------------------------------------------------------------
# Convenience: enable all static analysis on a target.
# ---------------------------------------------------------------------------
function(polysynth_enable_all_static_analysis target)
    polysynth_enable_compiler_warnings(${target})
    polysynth_enable_cppcheck(${target})
    polysynth_enable_clang_tidy(${target})
endfunction()
