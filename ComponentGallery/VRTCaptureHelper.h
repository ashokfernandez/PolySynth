#pragma once

// VRT Capture Abstraction
//
// Isolates all Skia surface readback logic in one place.  The rest of the
// codebase calls only the functions in the VRTCapture namespace; no Skia
// surface internals leak elsewhere.
//
// Preferred path  : IGraphicsSkia surface → SkPngEncoder
// Fallback path   : reports failure and returns false
//
// NOTE: The method name used to retrieve the backing surface from
// IGraphicsSkia (GetSurface() below) must be validated against
// external/iPlug2/IGraphics/IGraphicsSkia.h once dependencies are present.
// Run `just deps` to download iPlug2, then verify or update the call.

#include "IGraphics.h"

#include <cstdio>
#include <string>
#include <sys/stat.h>

#ifdef IGRAPHICS_SKIA
// IGraphicsSkia.h is in the iPlug2 IGraphics directory; the iPlug2 CMake
// config adds that directory to the include path automatically.
#include "IGraphicsSkia.h"
// Skia encode/core headers are transitively available after IGraphicsSkia.h.
// Explicit includes listed for clarity; adjust paths if Skia layout differs.
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkPixmap.h"
#include "SkStream.h"
#include "SkPngEncoder.h"
#endif  // IGRAPHICS_SKIA

namespace VRTCapture {

// ---------------------------------------------------------------------------
// Directory helpers
// ---------------------------------------------------------------------------

/// Recursively create `dir` (equivalent to mkdir -p).
/// Returns true on success or if the directory already exists.
inline bool EnsureDir(const std::string& dir) {
  if (dir.empty()) return false;
  // Walk the path segment by segment.
  for (size_t pos = 1; pos <= dir.size(); ++pos) {
    if (pos == dir.size() || dir[pos] == '/') {
      const std::string partial = dir.substr(0, pos);
      struct stat st{};
      if (stat(partial.c_str(), &st) != 0) {
        if (mkdir(partial.c_str(), 0755) != 0) return false;
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
inline void RenderWarmupFrames(iplug::igraphics::IGraphics* pGraphics,
                               int nFrames = 3) {
  if (!pGraphics) return;
  for (int i = 0; i < nFrames; ++i) {
    pGraphics->SetAllControlsDirty();
  }
}

// ---------------------------------------------------------------------------
// PNG capture
// ---------------------------------------------------------------------------

/// Save a rectangular region of the current IGraphics backing surface as PNG.
///
/// Preferred path  : Skia SkSurface readback via IGraphicsSkia (compile-
///                   time guarded by IGRAPHICS_SKIA).
/// Fallback path   : prints a diagnostic and returns false so callers know
///                   the capture was not completed.
///
/// @param pGraphics  Active IGraphics instance (must not be null).
/// @param bounds     Region to capture in plugin coordinates at 1× scale.
/// @param filePath   Absolute or repo-relative path for the output .png.
/// @return true on success.
inline bool SaveRegionAsPNG(iplug::igraphics::IGraphics* pGraphics,
                             const iplug::igraphics::IRECT& bounds,
                             const std::string& filePath) {
  if (!pGraphics) return false;

  const int x = static_cast<int>(bounds.L);
  const int y = static_cast<int>(bounds.T);
  const int w = static_cast<int>(bounds.W());
  const int h = static_cast<int>(bounds.H());

  if (w <= 0 || h <= 0) return false;

#ifdef IGRAPHICS_SKIA
  {
    using namespace iplug::igraphics;
    auto* pSkia = dynamic_cast<IGraphicsSkia*>(pGraphics);
    if (pSkia) {
      // NOTE: Validate GetSurface() against IGraphicsSkia.h.
      // Alternative names seen in iPlug2 forks: GetBackingSurface(), mSurface.
      sk_sp<SkSurface> surface = pSkia->GetSurface();
      if (surface) {
        // Software rasterizer: force CPU backend when possible so pixels are
        // deterministic across runs.  IGraphicsSkia may expose a method such
        // as SetForceCPURaster(true); call it here if available.

        // Allocate a destination bitmap for the cropped region.
        SkBitmap dstBitmap;
        if (!dstBitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(w, h))) {
          return false;
        }

        // Read pixels from the surface at offset (x, y) into dstBitmap.
        if (!surface->getCanvas()->readPixels(dstBitmap, x, y)) {
          return false;
        }

        // Encode to PNG via SkPngEncoder.
        SkFILEWStream stream(filePath.c_str());
        SkPixmap pixmap;
        if (!dstBitmap.peekPixels(&pixmap)) return false;
        SkPngEncoder::Options opts;
        return SkPngEncoder::Encode(&stream, pixmap, opts);
      }
    }
  }
#endif  // IGRAPHICS_SKIA

  // Fallback: Skia surface readback not available.
  // This happens when IGRAPHICS_SKIA is not defined (non-Skia backend) or
  // when GetSurface() returns nullptr.  Fix the TODO above once iPlug2
  // dependencies are present.
  fprintf(stderr,
          "[VRTCapture] WARNING: Skia surface readback unavailable for '%s'.\n"
          "  Ensure IGRAPHICS_SKIA is defined and verify IGraphicsSkia::GetSurface().\n",
          filePath.c_str());
  return false;
}

}  // namespace VRTCapture
