#include "space/physics/tac_barycentric.h" // self-inc
#include "src/common/math/tac_matrix3.h"
#include "src/common/math/tac_math.h"


namespace Tac
{

  Optional< BarycentricTriOutput > BarycentricTriangle( BarycentricTriInput input )
  {
    v3 v0 = input.mTri1 - input.mTri0;
    v3 v1 = input.mTri2 - input.mTri0;
    v3 v2 = input.mP - input.mTri0;
    float d00 = Dot( v0, v0 );
    float d01 = Dot( v0, v1 );
    float d11 = Dot( v1, v1 );
    float d20 = Dot( v2, v0 );
    float d21 = Dot( v2, v1 );
    float denom = d00 * d11 - d01 * d01;
    const bool fucked = Abs( denom ) < 0.000001f;
    if( fucked )
      return {};

    float invDenom = 1.0f / denom;
    float v = ( d11 * d20 - d01 * d21 ) * invDenom;
    float w = ( d00 * d21 - d01 * d20 ) * invDenom;
    float u = 1.0f - v - w;
    return BarycentricTriOutput{ u, v, w };
  }

  Optional< BarycentricTetOutput > BarycentricTetrahedron( BarycentricTetInput input )
  {
    v3 p = input.mP;
    v3 tet0 = input.mTet0;
    v3 tet1 = input.mTet1;
    v3 tet2 = input.mTet2;
    v3 tet3 = input.mTet3;

    const v3 a = tet1 - tet0;
    const v3 b = tet2 - tet0;
    const v3 c = tet3 - tet0;
    const v3 d = p - tet0;

    const m3 denominatorMatrix = m3::FromColumns( a, b, c );
    const float denominator = denominatorMatrix.determinant();
    const bool fucked = Abs( denominator ) < 0.001f;
    if( fucked )
      return {};

    const auto invDenominator = 1.0f / denominator;

    auto xNumeratorMatrix = m3::FromColumns( d, b, c );
    auto xNumerator = xNumeratorMatrix.determinant();
    auto x = xNumerator * invDenominator;

    auto yNumeratorMatrix = m3::FromColumns( a, d, c );
    auto yNumerator = yNumeratorMatrix.determinant();
    auto y = yNumerator * invDenominator;

    auto zNumeratorMatrix = m3::FromColumns( a, b, d );
    auto zNumerator = zNumeratorMatrix.determinant();
    auto z = zNumerator * invDenominator;

    return BarycentricTetOutput
    {
      .mBary0 = 1.0f - x - y - z,
      .mBary1 = x,
      .mBary2 = y,
      .mBary3 = z,
    };
  }

} // namespace Tac

