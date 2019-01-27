#pragma once
#include "common/math/tacVector4.h"

v4 TacGetColorSchemeA( float t );

void TacRGBToHSV( v3 inputRGB, float* h, float* s, float* v );
void TacHSVToRGB( float h, float s, float v, v3* outputRGB );
