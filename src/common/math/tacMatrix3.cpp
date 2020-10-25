#include "src/common/math/tacMatrix3.h"
#include "src/common/math/tacVector3.h"
#include <cmath> // sin, cos, sqrt

namespace Tac
{
  float* m3::data()
  {
    return &m00;
  }

  const float* m3::data() const
  {
    return &m00;
  }

  m3::m3( float mm00, float mm01, float mm02,
          float mm10, float mm11, float mm12,
          float mm20, float mm21, float mm22 )
    : m00( mm00 ), m01( mm01 ), m02( mm02 )
    , m10( mm10 ), m11( mm11 ), m12( mm12 )
    , m20( mm20 ), m21( mm21 ), m22( mm22 ) {}

  float m3::determinant() const
  {
    return
      m00 * ( m11 * m22 - m12 * m21 ) +
      m01 * ( m12 * m20 - m10 * m22 ) +
      m02 * ( m10 * m21 - m11 * m20 );
  }

  float& m3::operator()( int iRow, int iCol )
  {
    return data()[ 3 * iRow + iCol ];
  }

  float m3::operator()( int iRow, int iCol ) const
  {
    return data()[ 3 * iRow + iCol ];
  }

  float& m3::operator[]( int i )
  {
    return data()[ i ];
  }

  float m3::operator[]( int i ) const
  {
    return data()[ i ];
  }

  v3 operator * ( const m3& m, const v3& v )
  {
    v3 result;
    for( int r = 0; r < 3; ++r )
    {
      float sum = 0;
      for( int c = 0; c < 3; ++c )
      {
        sum += m( r, c ) * v[ c ];
      }
      result[ r ] = sum;
    }
    return result;
  }

  m3 operator * ( const m3& lhs, const m3& rhs )
  {
    m3 result;
    for( int r = 0; r < 3; ++r )
    {
      for( int c = 0; c < 3; ++c )
      {
        float sum = 0;
        for( int i = 0; i < 3; ++i )
        {
          sum += lhs( r, i ) * rhs( i, c );
        }
        result( r, c ) = sum;
      }
    }
    return result;
  }

  float determinant( const m3& m )
  {
    return m.determinant();
  }

  m3 m3::FromColumns( const v3& c0, const v3& c1, const v3& c2 )
  {
    return {
      c0.x, c1.x, c2.x,
      c0.y, c1.y, c2.y,
      c0.z, c1.z, c2.z };
  }

  m3 m3::FromRows( const v3& r0, const v3& r1, const v3& r2 )
  {
    return{
      r0.x, r0.y, r0.z,
      r1.x, r1.y, r1.z,
      r2.x, r2.y, r2.z };
  }

  m3 m3::Identity()
  {
    return {
      1, 0, 0,
      0, 1, 0,
      0, 0, 1 };
  }

  m3 m3::RotRadEuler( const v3& eulerRads )
  {
    const m3 result
      = m3::RotRadZ( eulerRads[ 2 ] )
      * m3::RotRadY( eulerRads[ 1 ] )
      * m3::RotRadX( eulerRads[ 0 ] );
    return result;
  }

  m3 m3::RotRadEulerInv( const v3& eulerRads )
  {
    // ( z y x )inv = xinv yinv zinv
    m3 result
      = m3::RotRadX( -eulerRads[ 0 ] )
      * m3::RotRadY( -eulerRads[ 1 ] )
      * m3::RotRadZ( -eulerRads[ 2 ] );
    return result;
  }

  m3 m3::RotRadX( float rotRad )
  {
    float s = sin( rotRad );
    float c = cos( rotRad );
    return {
      1, 0, 0,
      0, c, -s,
      0, s, c };
  }

  m3 m3::RotRadY( float rotRad )
  {
    float s = sin( rotRad );
    float c = cos( rotRad );
    return {
      c, 0, s,
      0, 1, 0,
      -s, 0, c };
  }

  m3 m3::RotRadZ( float rotRad )
  {
    float s = sin( rotRad );
    float c = cos( rotRad );
    return {
      c, -s, 0,
      s, c, 0,
      0, 0, 1 };
  }

  m3 m3::RotRadAngleAxis( const float angleRadians, const v3& unitAxis )
  {
    const float c = std::cos( angleRadians );
    const float s = std::sin( angleRadians );
    const float t = 1.0f - c;
    const float m00 = c + unitAxis[ 0 ] * unitAxis[ 0 ] * t;
    const float m11 = c + unitAxis[ 1 ] * unitAxis[ 1 ] * t;
    const float m22 = c + unitAxis[ 2 ] * unitAxis[ 2 ] * t;
    const float m10 = unitAxis[ 0 ] * unitAxis[ 1 ] * t + unitAxis[ 2 ] * s;
    const float m01 = unitAxis[ 0 ] * unitAxis[ 1 ] * t - unitAxis[ 2 ] * s;
    const float m20 = unitAxis[ 0 ] * unitAxis[ 2 ] * t - unitAxis[ 1 ] * s;
    const float m02 = unitAxis[ 0 ] * unitAxis[ 2 ] * t + unitAxis[ 1 ] * s;
    const float m21 = unitAxis[ 1 ] * unitAxis[ 2 ] * t + unitAxis[ 0 ] * s;
    const float m12 = unitAxis[ 1 ] * unitAxis[ 2 ] * t - unitAxis[ 0 ] * s;
    return
    {
      m00, m01, m02,
      m10, m11, m12,
      m20, m21, m22
    };
  }

  m3 m3::Scale( const v3& scale )
  {
    return {
      scale[ 0 ], 0, 0,
      0, scale[ 1 ], 0,
      0, 0, scale[ 2 ] };
  }

  m3 m3::Translate( const v2& v )
  {
    return {
      1, 0, v.x,
      0, 1, v.y,
      0, 0, 1 };
  }

}
