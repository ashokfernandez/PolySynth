#pragma once
#include <string>

struct IColor {
  int A, R, G, B;
  IColor(int a = 255, int r = 0, int g = 0, int b = 0)
      : A(a), R(r), G(g), B(b) {}
  IColor WithOpacity(float opacity) const {
    return IColor(static_cast<int>(A * opacity), R, G, B);
  }
};

struct IRECT {
  float L, T, R, B;
  IRECT(float l = 0, float t = 0, float r = 0, float b = 0)
      : L(l), T(t), R(r), B(b) {}

  float W() const { return R - L; }
  float H() const { return B - T; }
  float MW() const { return L + (W() * 0.5f); }
  float MH() const { return T + (H() * 0.5f); }

  IRECT GetPadded(float p) const { return IRECT(L - p, T - p, R + p, B + p); }
  IRECT GetMidVPadded(float pad) const { return IRECT(L, T + pad, R, B - pad); }
  IRECT GetFromTop(float h) const { return IRECT(L, T, R, T + h); }
  IRECT GetFromBottom(float h) const { return IRECT(L, B - h, R, B); }
  IRECT GetTranslated(float x, float y) const {
    return IRECT(L + x, T + y, R + x, B + y);
  }
  bool Contains(float x, float y) const {
    return x >= L && x <= R && y >= T && y <= B;
  }
};

#define COLOR_TRANSPARENT IColor(0, 0, 0, 0)
static const IColor COLOR_BLUE{255, 0, 0, 255};

struct IVStyle {};
static const IVStyle DEFAULT_STYLE;

enum EAlign { Near, Center, Far };

namespace iplug {}
namespace igraphics {}

struct IText {
  float size;
  IColor color;
  const char *font;
  EAlign align;
  IText(float s, IColor c, const char *f, EAlign a)
      : size(s), color(c), font(f), align(a) {}
};

class WDL_String {
public:
  WDL_String() : str("") {}
  WDL_String(const char *s) : str(s ? s : "") {}
  const char *Get() const { return str.c_str(); }
  int GetLength() const { return str.length(); }
  void Set(const char *s) { str = s ? s : ""; }
  void Append(const char *s) {
    if (s)
      str += s;
  }

private:
  std::string str;
};

#ifndef DegToRad
inline float DegToRad(float deg) { return deg * (3.14159f / 180.f); }
#endif
