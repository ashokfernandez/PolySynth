#!/bin/bash
# Serve docs/ over HTTP so the local Component Gallery Storybook works.

set -e
cd "$(dirname "$0")"

PORT=${1:-8092}
URL="http://127.0.0.1:${PORT}/docs/component-gallery/index.html"

if [ ! -f "docs/component-gallery/index.html" ]; then
  echo "Local gallery docs not found. Building them first..."
  ./build_gallery_pages.sh
fi

echo "Serving docs at ${URL}"
echo "Press Ctrl+C to stop."

if [ -z "${CI:-}" ] && [ -z "${NO_BROWSER:-}" ]; then
  if command -v open >/dev/null 2>&1; then
    (sleep 1; open "$URL" >/dev/null 2>&1 || true) &
  elif command -v xdg-open >/dev/null 2>&1; then
    (sleep 1; xdg-open "$URL" >/dev/null 2>&1 || true) &
  fi
fi

python3 -m http.server "$PORT" --bind 127.0.0.1
