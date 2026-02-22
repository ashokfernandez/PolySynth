#!/usr/bin/env python3
"""
Check that platform-specific iPlug2 APIs are properly guarded for web builds.

BundleResourcePath(path, PluginIDType) has the signature:
  - macOS/iOS: PluginIDType = const char*
  - Windows:   PluginIDType = HMODULE
  - Other:     PluginIDType = void*

On Emscripten (WEB_API / WAM_API), PluginIDType is void*, so calling
BundleResourcePath with a const char* argument fails to compile.
Any call to BundleResourcePath must be inside a #ifndef WEB_API guard.

Exit code: 0 if all guards are present, 1 if any unguarded calls found.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent
SOURCE_DIRS = ["src"]

# APIs that are unsafe to call with a const char* argument in WEB_API builds.
GUARDED_APIS = [
    "BundleResourcePath",
    "PluginPath",  # same signature, same issue
]

GUARD_OPEN_RE = re.compile(r"#\s*ifndef\s+WEB_API|#\s*if\s+.*!.*defined\s*\(\s*WEB_API\s*\)")
GUARD_CLOSE_RE = re.compile(r"#\s*endif")

API_CALL_RE = re.compile(r"\b(" + "|".join(re.escape(a) for a in GUARDED_APIS) + r")\s*\(")


def check_file(path: Path) -> list[str]:
    """Return list of violation strings for unguarded API calls."""
    violations = []
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    inside_guard = 0  # nesting depth of WEB_API guards
    brace_depth = 0   # track #if nesting for #endif matching

    # Simple single-pass state machine tracking preprocessor guard depth.
    # We track all #if/#ifdef/#ifndef to correctly match #endifs.
    any_if_re = re.compile(r"#\s*(if|ifdef|ifndef)\b")
    endif_re = re.compile(r"#\s*endif\b")

    guard_stack = []  # True = this level is a WEB_API guard, False = other

    for lineno, line in enumerate(lines, 1):
        stripped = line.strip()

        # Check if this line opens a WEB_API guard
        if GUARD_OPEN_RE.search(stripped):
            guard_stack.append(True)
            continue

        # Check if this line opens any other preprocessor block
        if any_if_re.match(stripped) and not GUARD_OPEN_RE.search(stripped):
            guard_stack.append(False)
            continue

        # Check if this line closes a preprocessor block
        if endif_re.match(stripped):
            if guard_stack:
                guard_stack.pop()
            continue

        # Skip comment-only lines
        if stripped.startswith("//") or stripped.startswith("*"):
            continue

        # Check for unguarded API calls
        if API_CALL_RE.search(stripped):
            # We're inside a WEB_API guard if any level in the stack is True
            in_web_guard = any(guard_stack)
            if not in_web_guard:
                match = API_CALL_RE.search(stripped)
                violations.append(
                    f"  {path}:{lineno}: unguarded call to {match.group(1)}() "
                    f"(must be inside #ifndef WEB_API)"
                )

    return violations


def main() -> int:
    all_violations = []

    for src_dir in SOURCE_DIRS:
        search_root = REPO_ROOT / src_dir
        if not search_root.exists():
            continue
        for cpp_file in search_root.rglob("*.cpp"):
            # Skip test files and generated files
            if "test" in cpp_file.name.lower():
                continue
            violations = check_file(cpp_file)
            all_violations.extend(violations)

    if all_violations:
        print("FAIL: Unguarded platform API calls found (will break WEB_API/Emscripten build):")
        for v in all_violations:
            print(v)
        print()
        print("Fix: wrap the call in #ifndef WEB_API ... #endif")
        return 1

    print(f"OK: All BundleResourcePath/PluginPath calls are properly guarded for WEB_API.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
