#pragma once
#include "src/common/math/tacVector4.h"
#include <cstdint>
namespace Tac
{

  v4 GetColorSchemeA( float t );

  void RGBToHSV( v3 inputRGB, float* h, float* s, float* v );
  void HSVToRGB( float h, float s, float v, v3* outputRGB );

  v3 HexToRGB( uint32_t hexColor );
}

