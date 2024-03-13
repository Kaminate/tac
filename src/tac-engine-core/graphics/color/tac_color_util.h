#pragma once

#include "tac-std-lib/tac_core.h"

#include "tac-std-lib/tac_ints.h"



namespace Tac
{
  v4   GetColorSchemeA( float t );

  void RGBToHSV( const v3& inputRGB, float* h, float* s, float* v );
  v3   RGBToHSV( const v3& );

  void HSVToRGB( float h, float s, float v, v3* outputRGB );
  v3   HSVToRGB( const v3& );

  v3   HexToRGB( u32 hexColor );
}

