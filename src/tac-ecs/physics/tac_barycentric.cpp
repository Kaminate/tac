#include "tac_barycentric.h" // self-inc
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_math.h"


namespace Tac
{

  Optional< BarycentricTriOutput > BarycentricTriangle( BarycentricTriInput input )
  {
    const v3 v0 { input.mTri1 - input.mTri0 };
    const v3 v1 { input.mTri2 - input.mTri0 };
    const v3 v2 { input.mP - input.mTri0 };
    const float d00 { Dot( v0, v0 ) };
    const float d01 { Dot( v0, v1 ) };
    const float d11 { Dot( v1, v1 ) };
    const float d20 { Dot( v2, v0 ) };
    const float d21 { Dot( v2, v1 ) };
    const float denom { d00 * d11 - d01 * d01 };
    const bool fucked { Abs( denom ) < 0.000001f };
    if( fucked )
      return {};

    const float invDenom { 1.0f / denom };
    const float v { ( d11 * d20 - d01 * d21 ) * invDenom };
    const float w { ( d00 * d21 - d01 * d20 ) * invDenom };
    const float u { 1.0f - v - w };
    return BarycentricTriOutput{ u, v, w };
  }

  Optional< BarycentricTetOutput > BarycentricTetrahedron( BarycentricTetInput input )
  {
    const v3 p    { input.mP };
    const v3 tet0 { input.mTet0 };
    const v3 tet1 { input.mTet1 };
    const v3 tet2 { input.mTet2 };
    const v3 tet3 { input.mTet3 };
    const v3 a    { tet1 - tet0 };
    const v3 b    { tet2 - tet0 };
    const v3 c    { tet3 - tet0 };
    const v3 d    { p - tet0 };

    const m3 denominatorMatrix { m3::FromColumns( a, b, c ) };
    const float denominator { denominatorMatrix.determinant() };
    const bool fucked { Abs( denominator ) < 0.001f };
    if( fucked )
      return {};

    const float invDenominator { 1.0f / denominator };
    const m3 xNumeratorMatrix  { m3::FromColumns( d, b, c ) };
    const float xNumerator     { xNumeratorMatrix.determinant() };
    const float x              { xNumerator * invDenominator };
    const m3 yNumeratorMatrix  { m3::FromColumns( a, d, c ) };
    const float yNumerator     { yNumeratorMatrix.determinant() };
    const float y              { yNumerator * invDenominator };
    const m3 zNumeratorMatrix  { m3::FromColumns( a, b, d ) };
    const float zNumerator     { zNumeratorMatrix.determinant() };
    const float z              { zNumerator * invDenominator };

    return BarycentricTetOutput
    {
      .mBary0 { 1.0f - x - y - z },
      .mBary1 { x },
      .mBary2 { y },
      .mBary3 { z },
    };
  }

} // namespace Tac

