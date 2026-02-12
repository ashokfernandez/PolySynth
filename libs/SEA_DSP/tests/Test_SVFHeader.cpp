#include "catch.hpp"
#include <sea_dsp/sea_svf.h>

TEST_CASE("SVFilter header is self-contained", "[Header][SelfContained][SVF]") {
  sea::SVFilter<double> filter;
  filter.Init(48000.0);
  filter.SetParams(1000.0, 0.707);
  double out = filter.ProcessLP(1.0);
  REQUIRE(out == out); // Not NaN
}
