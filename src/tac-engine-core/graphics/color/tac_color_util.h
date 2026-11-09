#pragma once

#include "tac-std-lib/tac_ints.h"

namespace Tac
{
  struct v3;
  struct v4;

  auto GetColorSchemeA( float t ) -> v4;
  void RGBToHSV( const v3& inputRGB, float* h, float* s, float* v );
  auto RGBToHSV( const v3& ) -> v3;
  void HSVToRGB( float h, float s, float v, v3* outputRGB );
  auto HSVToRGB( const v3& ) -> v3;
  auto HexToRGB( u32 hexColor ) -> v3;
}

