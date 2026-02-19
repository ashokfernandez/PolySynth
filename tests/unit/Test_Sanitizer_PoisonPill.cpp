/**
 * tests/unit/Test_Sanitizer_PoisonPill.cpp
 *
 * Sanitizer verification tests.
 *
 * PURPOSE
 * -------
 * These tests verify that the runtime sanitizer instrumentation is actually
 * working in a given build.  They are EXCLUDED from the default test suite
 * and must be run deliberately using the [poison-pill] tag:
 *
 *   ./run_tests "[poison-pill]"
 *
 * In a normal CI run (no USE_SANITIZER set) these tests are skipped via the
 * tag filter.  In the ASan/TSan CI jobs the tests are NOT invoked – the jobs
 * compile the sanitized binary and run the normal test suite to verify that
 * real code passes.  Use these poison-pill tests LOCALLY to confirm that
 * your toolchain's sanitizer actually fires before trusting it in CI.
 *
 * HOW TO USE
 * ----------
 * 1. Build with ASan:
 *      cmake -S tests -B tests/build_asan -DUSE_SANITIZER=Address -DCMAKE_BUILD_TYPE=Debug
 *      cmake --build tests/build_asan
 *
 * 2. Run ONLY the poison-pill tests (expect the process to abort):
 *      ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:print_stacktrace=1 \
 *      ./tests/build_asan/run_tests "[poison-pill][heap-oob]"
 *
 * 3. Confirm that ASan prints a report like:
 *      ==<pid>==ERROR: AddressSanitizer: heap-buffer-overflow on address ...
 *      READ of size 4 at 0x... thread T0
 *      #0 0x... in heap_out_of_bounds_read ...
 *
 * If ASan is NOT active the test catches the signal and fails with a message
 * explaining that the sanitizer did not fire.
 *
 * IMPORTANT
 * ---------
 * Do NOT add these tests to the UNIT_TEST_SOURCES list in CMakeLists.txt.
 * They are intentionally separate so they can be compiled as an optional
 * standalone binary for manual sanitizer validation only.
 */

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>
#include <atomic>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * Returns true when the binary was built with AddressSanitizer.
 * Uses the __has_feature() Clang extension where available; falls back to
 * the GCC __SANITIZE_ADDRESS__ macro.
 */
static bool isAsanActive()
{
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
    return true;
#  endif
#endif
#if defined(__SANITIZE_ADDRESS__)
    return true;
#endif
    return false;
}

/**
 * Returns true when the binary was built with ThreadSanitizer.
 */
static bool isTsanActive()
{
#if defined(__has_feature)
#  if __has_feature(thread_sanitizer)
    return true;
#  endif
#endif
#if defined(__SANITIZE_THREAD__)
    return true;
#endif
    return false;
}

// ---------------------------------------------------------------------------
// Poison-pill tests
//
// Each test performs a deliberately unsafe operation.  When a sanitizer is
// active the process is expected to abort with an error report.  When no
// sanitizer is active we skip the dangerous operation and FAIL the test with
// an explanatory message – this keeps the test suite from crashing in
// un-instrumented builds while still alerting developers that the
// verification didn't actually run.
// ---------------------------------------------------------------------------

TEST_CASE("Sanitizer self-check: ASan heap-out-of-bounds read",
          "[poison-pill][heap-oob][asan]")
{
    // This test verifies that ASan catches a one-past-the-end heap read.
    // Expected outcome when ASan is active:  process aborts with
    //   ERROR: AddressSanitizer: heap-buffer-overflow
    REQUIRE_MESSAGE(isAsanActive(),
        "ASan is NOT active in this build. "
        "Re-build with -DUSE_SANITIZER=Address to verify ASan detection. "
        "If you see this in a CI ASan job, the sanitizer flags were not applied.");

    // Deliberately read one element past the end of a heap allocation.
    // This is undefined behaviour that ASan catches reliably.
    int* arr = new int[4];
    for (int i = 0; i < 4; ++i) arr[i] = i;

    // The line below is the deliberate bug.  ASan will abort here.
    volatile int oob = arr[4]; // NOLINT(*-pointer-arithmetic)
    (void)oob;

    delete[] arr;

    // If we reach here without an ASan abort the test fails.
    FAIL("ASan did NOT catch the heap-out-of-bounds read. "
         "Check that -fsanitize=address is in both compile AND link flags.");
}

TEST_CASE("Sanitizer self-check: ASan use-after-free",
          "[poison-pill][use-after-free][asan]")
{
    REQUIRE_MESSAGE(isAsanActive(),
        "ASan is NOT active in this build. "
        "Re-build with -DUSE_SANITIZER=Address.");

    int* p = new int(42);
    delete p;

    // Deliberate use-after-free.
    volatile int uaf = *p; // NOLINT(*-use-after-free)
    (void)uaf;

    FAIL("ASan did NOT catch the use-after-free. "
         "Check sanitizer link flags.");
}

TEST_CASE("Sanitizer self-check: UBSan signed integer overflow",
          "[poison-pill][ubsan][signed-overflow]")
{
    // UBSan catches signed integer overflow (which is undefined behaviour in C++).
    // Expected outcome: process aborts with
    //   runtime error: signed integer overflow: INT_MAX + 1 cannot be represented
    REQUIRE_MESSAGE(isAsanActive(),
        "ASan/UBSan are NOT active in this build. "
        "Re-build with -DUSE_SANITIZER=Address (which also enables UBSan).");

    volatile int x = INT_MAX;
    // The addition below is signed integer overflow – UB in C++.
    volatile int overflow = x + 1; // NOLINT(*-integer-overflow)
    (void)overflow;

    FAIL("UBSan did NOT catch signed integer overflow. "
         "Check that -fsanitize=undefined is in both compile AND link flags.");
}

TEST_CASE("Sanitizer self-check: TSan data race",
          "[poison-pill][tsan][data-race]")
{
    // This test verifies that TSan catches an unprotected concurrent write.
    // Expected outcome: process aborts with
    //   WARNING: ThreadSanitizer: data race
    REQUIRE_MESSAGE(isTsanActive(),
        "TSan is NOT active in this build. "
        "Re-build with -DUSE_SANITIZER=Thread to verify TSan detection.");

    int shared = 0; // No mutex protecting this.

    std::thread writer([&shared]() {
        // Deliberate unsynchronised write.
        shared = 1; // NOLINT(*-race)
    });

    // Deliberate unsynchronised read while writer may be running.
    volatile int val = shared; // NOLINT(*-race)
    (void)val;

    writer.join();

    FAIL("TSan did NOT catch the data race. "
         "Check that -fsanitize=thread is in both compile AND link flags.");
}
