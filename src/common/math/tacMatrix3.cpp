#include "tacMatrix3.h"
#include <cmath> // sin, cos, sqrt

float* m3::data()
{
  return &m00;
}

const float* m3::data() const
{
  return &m00;
}

m3::m3(
  float mm00, float mm01, float mm02,
  float mm10, float mm11, float mm12,
  float mm20, float mm21, float mm22 )
{
  m00 = mm00;
  m01 = mm01;
  m02 = mm02;
  m10 = mm10;
  m11 = mm11;
  m12 = mm12;
  m20 = mm20;
  m21 = mm21;
  m22 = mm22;
}

m3 m3::FromColumns( const v3& c0, const v3& c1, const v3& c2 )
{
  return{
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

float m3::determinant() const
{
  return m00 * ( m11 * m22 - m12 * m21 )
    + m01 * ( m12 * m20 - m10 * m22 )
    + m02 * ( m10 * m21 - m11 * m20 );
}

// Make this static?
int m3::getValueIndex( int iRow, int iCol ) const
{
  return 3 * iRow + iCol;
}

float& m3::operator()( int iRow, int iCol )
{
  return data()[ getValueIndex( iRow, iCol ) ];
}
float m3::operator()( int iRow, int iCol ) const
{
  return data()[ getValueIndex( iRow, iCol ) ];
}


v3 operator * ( const m3& m, const v3& v )
{
  v3 result;
  for( int i = 0; i < 3; ++i )
  {
    float sum = 0;
    for( int j = 0; j < 3; ++j )
    {
      sum += m( i, j ) * v[ j ];
    }
    result[ i ] = sum;
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

m3 M3RotRadX( float rotRad )
{
  float s = sin( rotRad );
  float c = cos( rotRad );
  return {
    1, 0, 0,
    0, c, -s,
    0, s, c };
}
m3 M3RotRadY( float rotRad )
{
  float s = sin( rotRad );
  float c = cos( rotRad );
  return {
    c, 0, s,
    0, 1, 0,
    -s, 0, c };
}
m3 M3RotRadZ( float rotRad )
{
  float s = sin( rotRad );
  float c = cos( rotRad );
  return {
    c, -s, 0,
    s, c, 0,
    0, 0, 1 };
}
m3 M3AngleAxis( float angleRadians, v3 unitAxis )
{
  m3 m;
  float c = std::cos( angleRadians );
  float s = std::sin( angleRadians );
  float t = 1.0f - c;

  m( 0, 0 ) = c + unitAxis[ 0 ] * unitAxis[ 0 ] * t;
  m( 1, 1 ) = c + unitAxis[ 1 ] * unitAxis[ 1 ] * t;
  m( 2, 2 ) = c + unitAxis[ 2 ] * unitAxis[ 2 ] * t;


  float tmp1 = unitAxis[ 0 ] * unitAxis[ 1 ] * t;
  float tmp2 = unitAxis[ 2 ] * s;
  m( 1, 0 ) = tmp1 + tmp2;
  m( 0, 1 ) = tmp1 - tmp2;

  tmp1 = unitAxis[ 0 ] * unitAxis[ 2 ] * t;
  tmp2 = unitAxis[ 1 ] * s;
  m( 2, 0 ) = tmp1 - tmp2;
  m( 0, 2 ) = tmp1 + tmp2;

  tmp1 = unitAxis[ 1 ] * unitAxis[ 2 ] * t;
  tmp2 = unitAxis[ 0 ] * s;
  m( 2, 1 ) = tmp1 + tmp2;
  m( 1, 2 ) = tmp1 - tmp2;
  return m;
}
m3 M3Scale( v3 scale )
{
  m3 result =
  {
    scale[ 0 ], 0, 0,
    0, scale[ 1 ], 0,
    0, 0, scale[ 2 ],
  };
  return result;
}
m3 M3Scale( float x, float y, float z )
{
  return M3Scale( v3( x, y, z ) );
}

m3 M3Translate( float x, float y )
{
  return {
    1, 0, x,
    0, 1, y,
    0, 0, 1 };
}

m3 M3Translate( v2 v )
{
  return M3Translate( v.x, v.y );
}
