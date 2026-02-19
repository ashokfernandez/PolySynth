# cmake/Sanitizers.cmake
# Runtime sanitizer configuration for the PolySynth project.
#
# Exposes a USE_SANITIZER option that selects an instrumentation profile:
#
#   cmake -DUSE_SANITIZER=Address   # ASan + UBSan  (default test profile)
#   cmake -DUSE_SANITIZER=Thread    # TSan           (concurrency profile)
#   cmake -DUSE_SANITIZER=          # None           (release / production)
#
# IMPORTANT – The Sanitizer Matrix:
#   ASan and TSan use incompatible shadow-memory layouts and CANNOT be combined.
#   Always build in separate directories when switching profiles:
#
#     cmake -B build_asan  -DUSE_SANITIZER=Address
#     cmake -B build_tsan  -DUSE_SANITIZER=Thread
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/../cmake/Sanitizers.cmake)
#   polysynth_apply_sanitizers(run_tests)

cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------
# Public option – must be set before the first call to polysynth_apply_sanitizers.
# ---------------------------------------------------------------------------
set(USE_SANITIZER "" CACHE STRING
    "Enable runtime sanitizers. Values: Address | Thread | (empty = none)")
set_property(CACHE USE_SANITIZER PROPERTY STRINGS "" "Address" "Thread")

# ---------------------------------------------------------------------------
# Internal: validate that the toolchain supports the requested sanitizer.
# ---------------------------------------------------------------------------
function(_polysynth_check_sanitizer_support sanitizer_flag out_supported)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        set(${out_supported} TRUE PARENT_SCOPE)
    else()
        message(WARNING
            "[Sanitizers] Sanitizers require GCC or Clang. "
            "The current compiler (${CMAKE_CXX_COMPILER_ID}) may not support them. "
            "Proceeding anyway – expect potential linker errors.")
        set(${out_supported} TRUE PARENT_SCOPE)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Apply sanitizer flags to a target.
# Call AFTER add_executable / add_library.
# ---------------------------------------------------------------------------
function(polysynth_apply_sanitizers target)
    if(USE_SANITIZER STREQUAL "")
        return()  # Nothing to do; clean build.
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        message(WARNING
            "[Sanitizers] USE_SANITIZER='${USE_SANITIZER}' is set but CMAKE_BUILD_TYPE "
            "is '${CMAKE_BUILD_TYPE}'. Sanitizers produce much better reports with "
            "CMAKE_BUILD_TYPE=Debug.")
    endif()

    if(USE_SANITIZER STREQUAL "Address")
        _polysynth_check_sanitizer_support("address" _ok)
        if(_ok)
            message(STATUS
                "[Sanitizers] Enabling ASan + UBSan on '${target}'")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer   # Required for readable stack traces
                -g                        # Debug symbols for symbolizer
                -fno-optimize-sibling-calls  # Better stack traces through tail calls
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address,undefined
            )
            # Ensure the test binary surfaces leaks and halts immediately.
            # These environment variables are consumed by ASan at runtime.
            # They are set here as target properties so run_analysis.py / ctest
            # can read them without hard-coding them in the CI yaml.
            set_target_properties(${target} PROPERTIES
                ASAN_OPTIONS "detect_leaks=1:halt_on_error=1:print_stacktrace=1"
                UBSAN_OPTIONS "print_stacktrace=1:halt_on_error=1"
            )
        endif()

    elseif(USE_SANITIZER STREQUAL "Thread")
        _polysynth_check_sanitizer_support("thread" _ok)
        if(_ok)
            message(STATUS
                "[Sanitizers] Enabling TSan on '${target}'")
            # TSan is mutually exclusive with ASan – never combine.
            target_compile_options(${target} PRIVATE
                -fsanitize=thread
                -fno-omit-frame-pointer
                -g
            )
            target_link_options(${target} PRIVATE
                -fsanitize=thread
            )
            set_target_properties(${target} PROPERTIES
                TSAN_OPTIONS "halt_on_error=1:print_stacktrace=1"
            )
        endif()

    else()
        message(FATAL_ERROR
            "[Sanitizers] Unknown USE_SANITIZER value: '${USE_SANITIZER}'. "
            "Valid values: Address, Thread, or empty string.")
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Convenience: print a summary of the active sanitizer profile.
# Call once, typically at the end of the root CMakeLists.txt.
# ---------------------------------------------------------------------------
function(polysynth_sanitizer_summary)
    if(USE_SANITIZER STREQUAL "")
        message(STATUS "[Sanitizers] No runtime sanitizers active.")
    elseif(USE_SANITIZER STREQUAL "Address")
        message(STATUS "[Sanitizers] Profile: ASan + UBSan")
        message(STATUS "             ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:print_stacktrace=1")
        message(STATUS "             UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1")
        message(STATUS "             To suppress 3rd-party leaks: LSAN_OPTIONS=suppressions=tools/lsan.supp")
    elseif(USE_SANITIZER STREQUAL "Thread")
        message(STATUS "[Sanitizers] Profile: TSan")
        message(STATUS "             TSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
    endif()
endfunction()
