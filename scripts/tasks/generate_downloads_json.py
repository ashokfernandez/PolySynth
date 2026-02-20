#!/usr/bin/env python3
"""Generate docs/downloads.json from the latest GitHub release."""

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


def fetch_latest_release(repo: str, token: str | None) -> dict[str, Any] | None:
    url = f"https://api.github.com/repos/{repo}/releases/latest"
    headers = {
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "polysynth-downloads-json",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = Request(url, headers=headers)
    try:
        with urlopen(req, timeout=20) as response:
            return json.loads(response.read().decode("utf-8"))
    except HTTPError as err:
        if err.code == 404:
            return None
        raise
    except URLError as err:
        raise RuntimeError(f"Failed to fetch latest release for {repo}: {err}") from err


def classify_platform_assets(assets: list[dict[str, Any]]) -> dict[str, str]:
    platform_urls: dict[str, str] = {}

    for asset in assets:
        name = str(asset.get("name", "")).lower()
        url = str(asset.get("browser_download_url", ""))
        if not url:
            continue

        if "mac" in name or name.endswith(".pkg"):
            platform_urls.setdefault("macos", url)
        if "win" in name or "windows" in name:
            platform_urls.setdefault("windows", url)
        if name.endswith(".zip") and "vst3" in name and "windows" not in platform_urls:
            platform_urls.setdefault("windows", url)

    return platform_urls


def build_payload(repo: str, latest: dict[str, Any] | None) -> dict[str, Any]:
    payload: dict[str, Any] = {
        "generated_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "repository": repo,
        "release": None,
        "assets": [],
        "platforms": {},
    }

    if latest is None:
        return payload

    assets = latest.get("assets", []) or []
    normalized_assets = []
    for asset in assets:
        normalized_assets.append(
            {
                "name": asset.get("name"),
                "size_bytes": asset.get("size"),
                "content_type": asset.get("content_type"),
                "download_url": asset.get("browser_download_url"),
            }
        )

    payload["release"] = {
        "tag": latest.get("tag_name"),
        "name": latest.get("name"),
        "published_at": latest.get("published_at"),
        "html_url": latest.get("html_url"),
    }
    payload["assets"] = normalized_assets
    payload["platforms"] = classify_platform_assets(assets)
    return payload


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate downloads.json from latest GitHub release")
    parser.add_argument("--output", required=True, help="Output JSON path (e.g. docs/downloads.json)")
    parser.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", ""))
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not args.repo:
        print("Missing --repo and GITHUB_REPOSITORY is not set.", file=sys.stderr)
        return 2

    token = os.environ.get("GITHUB_TOKEN")
    latest = fetch_latest_release(args.repo, token)
    payload = build_payload(args.repo, latest)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
