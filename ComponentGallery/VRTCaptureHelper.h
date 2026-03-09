#pragma once

// VRT Capture Abstraction
//
// Isolates all Skia surface readback logic in one place.  The rest of the
// codebase calls only the functions in the VRTCapture namespace; no Skia
// surface internals leak elsewhere.
//
// Skia path: reads pixels from the SkCanvas via GetDrawContext(), crops to
// component bounds, encodes to PNG via SkPngEncoder. Deterministic.

#include "IGraphics.h"

#include <cstdio>
#include <string>
#include <sys/stat.h>

#ifndef IGRAPHICS_SKIA
#error "VRTCaptureHelper requires IGRAPHICS_SKIA — set the backend to SKIA in CMakeLists.txt"
#endif

#include "IGraphicsSkia.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPixmap.h"
#include "include/encode/SkPngEncoder.h"
#include "include/core/SkStream.h"

namespace VRTCapture {

// ---------------------------------------------------------------------------
// Directory helpers
// ---------------------------------------------------------------------------

/// Recursively create `dir` (equivalent to mkdir -p).
/// Returns true on success or if the directory already exists.
inline bool EnsureDir(const std::string &dir) {
  if (dir.empty())
    return false;
  // Walk the path segment by segment.
  for (size_t pos = 1; pos <= dir.size(); ++pos) {
    if (pos == dir.size() || dir[pos] == '/') {
      const std::string partial = dir.substr(0, pos);
      struct stat st {};
      if (stat(partial.c_str(), &st) != 0) {
        if (mkdir(partial.c_str(), 0755) != 0)
          return false;
      }
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// Warmup helper
// ---------------------------------------------------------------------------

/// Mark all controls dirty to flush animation / first-paint transients.
/// Call before capturing each component to ensure a stable rendered frame.
inline void RenderWarmupFrames(iplug::igraphics::IGraphics *pGraphics,
                               int nFrames = 3) {
  if (!pGraphics)
    return;
  for (int i = 0; i < nFrames; ++i) {
    pGraphics->SetAllControlsDirty();
  }
}

// ---------------------------------------------------------------------------
// PNG capture
// ---------------------------------------------------------------------------

/// Save a rectangular region of the current IGraphics backing surface as PNG.
///
/// Reads pixels from the Skia canvas (obtained via GetDrawContext()), crops
/// to the specified component bounds, and encodes directly to PNG. This
/// produces deterministic output independent of window position or OS chrome.
///
/// @param pGraphics  Active IGraphics instance (must not be null).
/// @param bounds     Region to capture in plugin coordinates at 1× scale.
/// @param filePath   Absolute or repo-relative path for the output .png.
/// @return true on success.
inline bool SaveRegionAsPNG(iplug::igraphics::IGraphics *pGraphics,
                            const iplug::igraphics::IRECT &bounds,
                            const std::string &filePath) {
  if (!pGraphics)
    return false;

  const float scale = pGraphics->GetDrawScale() * pGraphics->GetScreenScale();
  const int x = static_cast<int>(bounds.L * scale);
  const int y = static_cast<int>(bounds.T * scale);
  const int w = static_cast<int>(bounds.W() * scale);
  const int h = static_cast<int>(bounds.H() * scale);

  if (w <= 0 || h <= 0)
    return false;

  auto *canvas = static_cast<SkCanvas *>(pGraphics->GetDrawContext());
  if (!canvas) {
    fprintf(stderr, "[VRTCapture] GetDrawContext() returned null for '%s'.\n",
            filePath.c_str());
    return false;
  }

  SkBitmap dstBitmap;
  if (!dstBitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(w, h))) {
    return false;
  }

  if (!canvas->readPixels(dstBitmap, x, y)) {
    fprintf(stderr,
            "[VRTCapture] readPixels failed for '%s' "
            "(region %d,%d %dx%d).\n",
            filePath.c_str(), x, y, w, h);
    return false;
  }

  SkFILEWStream stream(filePath.c_str());
  SkPixmap pixmap;
  if (!dstBitmap.peekPixels(&pixmap))
    return false;
  SkPngEncoder::Options opts;
  return SkPngEncoder::Encode(&stream, pixmap, opts);
}

} // namespace VRTCapture
