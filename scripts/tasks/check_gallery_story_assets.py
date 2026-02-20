#!/usr/bin/env python3
"""Validate that Storybook gallery iframe targets exist in ComponentGallery/build-web."""

from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    stories_path = root / "tests" / "Visual" / "stories" / "component-gallery.stories.js"
    build_root = root / "ComponentGallery" / "build-web"

    story_src = stories_path.read_text(encoding="utf-8")
    components = sorted(set(re.findall(r"createGalleryFrame\('([^']+)'\)", story_src)))

    if not components:
        print(f"[gallery-check] No component gallery stories found in {stories_path}")
        return 1

    missing = []
    for component in components:
        page = build_root / component / "index.html"
        if not page.is_file():
            missing.append(str(page))

    if missing:
        print("[gallery-check] Missing component gallery pages:")
        for path in missing:
            print(f"  - {path}")
        return 1

    print(f"[gallery-check] OK: {len(components)} component pages available in {build_root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
