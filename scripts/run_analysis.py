#!/usr/bin/env python3
"""
scripts/run_analysis.py
=======================
CI wrapper for PolySynth static analysis.

Runs Cppcheck and (optionally) Clang-Tidy outside the CMake build step.
This script is invoked by the "lint-static-analysis" CI job and can also
be run locally for a quick pre-commit check.

Usage:
    python3 scripts/run_analysis.py --build-dir tests/build [--tool cppcheck|clang-tidy|all]

Exit codes:
    0   All checks passed.
    1   One or more checks failed (details printed to stdout in compiler-error format).
    2   Script misconfiguration (missing tools, bad arguments).

Output contract:
    - Human output: compiler-error format  -> file:line: [id] message
    - CI artifact:  cppcheck-report.xml    -> uploaded by the CI job
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent


def _require_tool(name: str, candidates: list[str] | None = None) -> str:
    """Return path to the first found candidate, or abort."""
    for candidate in (candidates or [name]):
        path = shutil.which(candidate)
        if path:
            return path
    print(
        f"[FATAL] Required tool not found: {name}\n"
        f"        Tried: {candidates or [name]}\n"
        f"        Install with: apt-get install {name}  OR  brew install {name}",
        file=sys.stderr,
    )
    sys.exit(2)


def _compile_commands_path(build_dir: Path) -> Path:
    """Locate compile_commands.json, aborting if absent."""
    cc = build_dir / "compile_commands.json"
    if not cc.exists():
        print(
            f"[FATAL] compile_commands.json not found at {cc}\n"
            "        Re-run CMake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            file=sys.stderr,
        )
        sys.exit(2)
    return cc


def _source_files_from_compile_commands(cc_path: Path) -> list[str]:
    """Extract the list of .cpp/.c files from the compile database."""
    with cc_path.open() as f:
        entries = json.load(f)
    files = []
    for entry in entries:
        src = entry.get("file", "")
        # Skip 3rd-party files
        skip_patterns = ("external/", "iPlug2/", "daisysp/", "/build/")
        if any(p in src for p in skip_patterns):
            continue
        if src.endswith((".cpp", ".c", ".cc", ".cxx")):
            files.append(src)
    return files


# ---------------------------------------------------------------------------
# Cppcheck runner
# ---------------------------------------------------------------------------

def run_cppcheck(build_dir: Path, verbose: bool = False) -> int:
    """Run Cppcheck using the compile database. Returns exit code."""
    cppcheck = _require_tool("cppcheck")
    cc_path = _compile_commands_path(build_dir)

    suppressions = REPO_ROOT / "tools" / "suppressions.txt"
    xml_output = build_dir / "cppcheck-report.xml"

    cmd = [
        cppcheck,
        "--enable=warning,performance,portability,style",
        "--inconclusive",
        f"--project={cc_path}",
        "--template={file}:{line}: [{id}] {message}",
        "--error-exitcode=1",
        "--xml",
        "--xml-version=2",
        f"--output-file={xml_output}",
    ]

    if suppressions.exists():
        cmd.append(f"--suppressions-list={suppressions}")

    print(f"[Cppcheck] Running analysis (compile db: {cc_path})")
    if verbose:
        print(f"           Command: {' '.join(str(c) for c in cmd)}")

    result = subprocess.run(cmd, capture_output=False)

    if result.returncode == 0:
        print(f"[Cppcheck] PASSED  (XML report: {xml_output})")
    else:
        print(f"[FAIL]     Cppcheck found issues. XML report: {xml_output}")

    return result.returncode


# ---------------------------------------------------------------------------
# Clang-Tidy runner
# ---------------------------------------------------------------------------

def run_clang_tidy(build_dir: Path, verbose: bool = False) -> int:
    """Run Clang-Tidy on all project source files. Returns exit code."""
    clang_tidy = _require_tool(
        "clang-tidy",
        ["clang-tidy", "clang-tidy-18", "clang-tidy-17", "clang-tidy-16", "clang-tidy-15"],
    )
    cc_path = _compile_commands_path(build_dir)
    config_file = REPO_ROOT / "tools" / ".clang-tidy"

    source_files = _source_files_from_compile_commands(cc_path)
    if not source_files:
        print("[Clang-Tidy] No source files found in compile_commands.json – skipping.")
        return 0

    print(f"[Clang-Tidy] Analysing {len(source_files)} source file(s)")

    # Run clang-tidy in parallel using run-clang-tidy.py if available,
    # otherwise fall back to serial execution.
    run_parallel = shutil.which("run-clang-tidy") or shutil.which("run-clang-tidy.py")

    overall_rc = 0

    if run_parallel:
        cmd = [
            run_parallel,
            f"-clang-tidy-binary={clang_tidy}",
            f"-p={build_dir}",
        ]
        if config_file.exists():
            # run-clang-tidy does not support --config-file directly; the
            # config file is picked up automatically from the source tree.
            pass
        if verbose:
            print(f"           Parallel command: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=False)
        overall_rc = result.returncode
    else:
        for src in source_files:
            cmd = [
                clang_tidy,
                f"-p={build_dir}",
                "--warnings-as-errors=*",
            ]
            if config_file.exists():
                cmd.append(f"--config-file={config_file}")
            cmd.append(src)

            if verbose:
                print(f"           Checking: {src}")

            result = subprocess.run(cmd, capture_output=False)
            if result.returncode != 0:
                overall_rc = result.returncode

    if overall_rc == 0:
        print("[Clang-Tidy] PASSED")
    else:
        print("[FAIL]       Clang-Tidy found issues.")

    return overall_rc


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run static analysis tools for the PolySynth project.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--build-dir",
        default="tests/build",
        help="Path to the CMake build directory containing compile_commands.json "
             "(default: tests/build)",
    )
    parser.add_argument(
        "--tool",
        choices=["cppcheck", "clang-tidy", "all"],
        default="all",
        help="Which tool(s) to run (default: all)",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print full tool invocations",
    )
    args = parser.parse_args()

    build_dir = Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = REPO_ROOT / build_dir

    exit_codes: list[int] = []

    if args.tool in ("cppcheck", "all"):
        exit_codes.append(run_cppcheck(build_dir, verbose=args.verbose))

    if args.tool in ("clang-tidy", "all"):
        exit_codes.append(run_clang_tidy(build_dir, verbose=args.verbose))

    # Summary
    print()
    if all(rc == 0 for rc in exit_codes):
        print("[PASS] All static analysis checks passed.")
        return 0
    else:
        print("[FAIL] One or more static analysis checks FAILED. See output above.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
