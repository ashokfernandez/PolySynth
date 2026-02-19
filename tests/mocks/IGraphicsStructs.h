#pragma once
#include <string>

struct IColor {
  int A, R, G, B;
  IColor(int a = 255, int r = 0, int g = 0, int b = 0)
      : A(a), R(r), G(g), B(b) {}
};

struct IRECT {
  float L, T, R, B;
  IRECT(float l = 0, float t = 0, float r = 0, float b = 0)
      : L(l), T(t), R(r), B(b) {}
  IRECT GetPadded(float p) const {
    return IRECT(L + p, T + p, R - p, B - p);
  } // Mock logic
  IRECT GetFromTop(float h) const { return IRECT(L, T, R, T + h); }
  IRECT GetTranslated(float x, float y) const {
    return IRECT(L + x, T + y, R + x, B + y);
  }
};

#define COLOR_TRANSPARENT IColor(0, 0, 0, 0)

enum EAlign { Near, Center, Far };

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
  WDL_String(const char *s) : str(s ? s : "") {}
  const char *Get() const { return str.c_str(); }
  int GetLength() const { return str.length(); }

private:
  std::string str;
};
