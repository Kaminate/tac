#include "src/common/math/tacMatrix4.h"
#include <cmath>
#include <cstring> // memcmp

namespace Tac
{


float* m4::data() { return mValues; }
const float* m4::data() const { return mValues; }

m4::m4( const m3& m )
{
  *this = {
    m( 0, 0 ), m( 0, 1 ), m( 0, 2 ), 0,
    m( 1, 0 ), m( 1, 1 ), m( 1, 2 ), 0,
    m( 2, 0 ), m( 2, 1 ), m( 2, 2 ), 0,
    0, 0, 0, 1 };
}

m4::m4(
  float m00, float m01, float m02, float m03,
  float m10, float m11, float m12, float m13,
  float m20, float m21, float m22, float m23,
  float m30, float m31, float m32, float m33 )
{
  mValues[ 0 ] = m00;
  mValues[ 1 ] = m01;
  mValues[ 2 ] = m02;
  mValues[ 3 ] = m03;

  mValues[ 4 ] = m10;
  mValues[ 5 ] = m11;
  mValues[ 6 ] = m12;
  mValues[ 7 ] = m13;

  mValues[ 8 ] = m20;
  mValues[ 9 ] = m21;
  mValues[ 10 ] = m22;
  mValues[ 11 ] = m23;

  mValues[ 12 ] = m30;
  mValues[ 13 ] = m31;
  mValues[ 14 ] = m32;
  mValues[ 15 ] = m33;
}

// Make this static?
int m4::getValueIndex( int iRow, int iCol ) const { return 4 * iRow + iCol; }
float& m4::operator()( int iRow, int iCol ) { return mValues[ getValueIndex( iRow, iCol ) ]; }
float m4::operator()( int iRow, int iCol ) const { return mValues[ getValueIndex( iRow, iCol ) ]; }
float& m4::operator[]( int i ) { return mValues[ i ]; }
float m4::operator[]( int i ) const { return mValues[ i ]; }
void m4::operator /= ( float f )
{
  for( float& v : mValues )
    v *= 1.0f / f;
}
bool m4::operator == ( const m4& m ) const
{
  bool result = 0 == std::memcmp( data(), m.data(), sizeof( m4 ) );
  return result;
}

m4 m4::Identity()
{
  return {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1 };
}


v4 operator * ( const m4& m, const v4& v )
{
  v4 result;
  for( int i = 0; i < 4; ++i )
  {
    float sum = 0;
    for( int j = 0; j < 4; ++j )
    {
      sum += m( i, j ) * v[ j ];
    }
    result[ i ] = sum;
  }
  return result;
}

m4 operator * ( const m4& lhs, const m4& rhs )
{
  m4 result;
  for( int r = 0; r < 4; ++r )
  {
    for( int c = 0; c < 4; ++c )
    {
      float sum = 0;
      for( int i = 0; i < 4; ++i )
      {
        sum += lhs( r, i ) * rhs( i, c );
      }
      result( r, c ) = sum;
    }
  }
  return result;
}


m4 M4Translate( v3 translate )
{
  return {
    1, 0, 0, translate[ 0 ],
    0, 1, 0, translate[ 1 ],
    0, 0, 1, translate[ 2 ],
    0, 0, 0, 1 };
}
m4 M4Translate( float x, float y, float z )
{
  return M4Translate( v3( x, y, z ) );
}
m4 M4RotRadX( float rotRad )
{
  return M3RotRadX( rotRad );
}
m4 M4RotRadY( float rotRad )
{
  return M3RotRadY( rotRad );
}
m4 M4RotRadZ( float rotRad )
{
  return M3RotRadZ( rotRad );
}
m4 M4Transform( v3 scale, m3 rot, v3 translate )
{
  float m00 = scale[ 0 ] * rot( 0, 0 );
  float m01 = scale[ 1 ] * rot( 0, 1 );
  float m02 = scale[ 2 ] * rot( 0, 2 );

  float m10 = scale[ 0 ] * rot( 1, 0 );
  float m11 = scale[ 1 ] * rot( 1, 1 );
  float m12 = scale[ 2 ] * rot( 1, 2 );

  float m20 = scale[ 0 ] * rot( 2, 0 );
  float m21 = scale[ 1 ] * rot( 2, 1 );
  float m22 = scale[ 2 ] * rot( 2, 2 );

  return {
    m00, m01, m02, translate[ 0 ],
    m10, m11, m12, translate[ 1 ],
    m20, m21, m22, translate[ 2 ],
    0, 0, 0, 1 };
}
m4 M4Transform( v3 scale, v3 eulerRads, v3 translate )
{
  m3 rot = M3RotRad( eulerRads );
  return M4Transform( scale, rot, translate );
}
void M4Inverse( const m4& m, m4* result, bool* resultExists )
{
  // gluInvertMatrix
  m4 inv;
  inv[ 0 ] = m[ 5 ] * m[ 10 ] * m[ 15 ] -
    m[ 5 ] * m[ 11 ] * m[ 14 ] -
    m[ 9 ] * m[ 6 ] * m[ 15 ] +
    m[ 9 ] * m[ 7 ] * m[ 14 ] +
    m[ 13 ] * m[ 6 ] * m[ 11 ] -
    m[ 13 ] * m[ 7 ] * m[ 10 ];

  inv[ 4 ] = -m[ 4 ] * m[ 10 ] * m[ 15 ] +
    m[ 4 ] * m[ 11 ] * m[ 14 ] +
    m[ 8 ] * m[ 6 ] * m[ 15 ] -
    m[ 8 ] * m[ 7 ] * m[ 14 ] -
    m[ 12 ] * m[ 6 ] * m[ 11 ] +
    m[ 12 ] * m[ 7 ] * m[ 10 ];

  inv[ 8 ] = m[ 4 ] * m[ 9 ] * m[ 15 ] -
    m[ 4 ] * m[ 11 ] * m[ 13 ] -
    m[ 8 ] * m[ 5 ] * m[ 15 ] +
    m[ 8 ] * m[ 7 ] * m[ 13 ] +
    m[ 12 ] * m[ 5 ] * m[ 11 ] -
    m[ 12 ] * m[ 7 ] * m[ 9 ];

  inv[ 12 ] = -m[ 4 ] * m[ 9 ] * m[ 14 ] +
    m[ 4 ] * m[ 10 ] * m[ 13 ] +
    m[ 8 ] * m[ 5 ] * m[ 14 ] -
    m[ 8 ] * m[ 6 ] * m[ 13 ] -
    m[ 12 ] * m[ 5 ] * m[ 10 ] +
    m[ 12 ] * m[ 6 ] * m[ 9 ];

  inv[ 1 ] = -m[ 1 ] * m[ 10 ] * m[ 15 ] +
    m[ 1 ] * m[ 11 ] * m[ 14 ] +
    m[ 9 ] * m[ 2 ] * m[ 15 ] -
    m[ 9 ] * m[ 3 ] * m[ 14 ] -
    m[ 13 ] * m[ 2 ] * m[ 11 ] +
    m[ 13 ] * m[ 3 ] * m[ 10 ];

  inv[ 5 ] = m[ 0 ] * m[ 10 ] * m[ 15 ] -
    m[ 0 ] * m[ 11 ] * m[ 14 ] -
    m[ 8 ] * m[ 2 ] * m[ 15 ] +
    m[ 8 ] * m[ 3 ] * m[ 14 ] +
    m[ 12 ] * m[ 2 ] * m[ 11 ] -
    m[ 12 ] * m[ 3 ] * m[ 10 ];

  inv[ 9 ] = -m[ 0 ] * m[ 9 ] * m[ 15 ] +
    m[ 0 ] * m[ 11 ] * m[ 13 ] +
    m[ 8 ] * m[ 1 ] * m[ 15 ] -
    m[ 8 ] * m[ 3 ] * m[ 13 ] -
    m[ 12 ] * m[ 1 ] * m[ 11 ] +
    m[ 12 ] * m[ 3 ] * m[ 9 ];

  inv[ 13 ] = m[ 0 ] * m[ 9 ] * m[ 14 ] -
    m[ 0 ] * m[ 10 ] * m[ 13 ] -
    m[ 8 ] * m[ 1 ] * m[ 14 ] +
    m[ 8 ] * m[ 2 ] * m[ 13 ] +
    m[ 12 ] * m[ 1 ] * m[ 10 ] -
    m[ 12 ] * m[ 2 ] * m[ 9 ];

  inv[ 2 ] = m[ 1 ] * m[ 6 ] * m[ 15 ] -
    m[ 1 ] * m[ 7 ] * m[ 14 ] -
    m[ 5 ] * m[ 2 ] * m[ 15 ] +
    m[ 5 ] * m[ 3 ] * m[ 14 ] +
    m[ 13 ] * m[ 2 ] * m[ 7 ] -
    m[ 13 ] * m[ 3 ] * m[ 6 ];

  inv[ 6 ] = -m[ 0 ] * m[ 6 ] * m[ 15 ] +
    m[ 0 ] * m[ 7 ] * m[ 14 ] +
    m[ 4 ] * m[ 2 ] * m[ 15 ] -
    m[ 4 ] * m[ 3 ] * m[ 14 ] -
    m[ 12 ] * m[ 2 ] * m[ 7 ] +
    m[ 12 ] * m[ 3 ] * m[ 6 ];

  inv[ 10 ] = m[ 0 ] * m[ 5 ] * m[ 15 ] -
    m[ 0 ] * m[ 7 ] * m[ 13 ] -
    m[ 4 ] * m[ 1 ] * m[ 15 ] +
    m[ 4 ] * m[ 3 ] * m[ 13 ] +
    m[ 12 ] * m[ 1 ] * m[ 7 ] -
    m[ 12 ] * m[ 3 ] * m[ 5 ];

  inv[ 14 ] = -m[ 0 ] * m[ 5 ] * m[ 14 ] +
    m[ 0 ] * m[ 6 ] * m[ 13 ] +
    m[ 4 ] * m[ 1 ] * m[ 14 ] -
    m[ 4 ] * m[ 2 ] * m[ 13 ] -
    m[ 12 ] * m[ 1 ] * m[ 6 ] +
    m[ 12 ] * m[ 2 ] * m[ 5 ];

  inv[ 3 ] = -m[ 1 ] * m[ 6 ] * m[ 11 ] +
    m[ 1 ] * m[ 7 ] * m[ 10 ] +
    m[ 5 ] * m[ 2 ] * m[ 11 ] -
    m[ 5 ] * m[ 3 ] * m[ 10 ] -
    m[ 9 ] * m[ 2 ] * m[ 7 ] +
    m[ 9 ] * m[ 3 ] * m[ 6 ];

  inv[ 7 ] = m[ 0 ] * m[ 6 ] * m[ 11 ] -
    m[ 0 ] * m[ 7 ] * m[ 10 ] -
    m[ 4 ] * m[ 2 ] * m[ 11 ] +
    m[ 4 ] * m[ 3 ] * m[ 10 ] +
    m[ 8 ] * m[ 2 ] * m[ 7 ] -
    m[ 8 ] * m[ 3 ] * m[ 6 ];

  inv[ 11 ] = -m[ 0 ] * m[ 5 ] * m[ 11 ] +
    m[ 0 ] * m[ 7 ] * m[ 9 ] +
    m[ 4 ] * m[ 1 ] * m[ 11 ] -
    m[ 4 ] * m[ 3 ] * m[ 9 ] -
    m[ 8 ] * m[ 1 ] * m[ 7 ] +
    m[ 8 ] * m[ 3 ] * m[ 5 ];

  inv[ 15 ] = m[ 0 ] * m[ 5 ] * m[ 10 ] -
    m[ 0 ] * m[ 6 ] * m[ 9 ] -
    m[ 4 ] * m[ 1 ] * m[ 10 ] +
    m[ 4 ] * m[ 2 ] * m[ 9 ] +
    m[ 8 ] * m[ 1 ] * m[ 6 ] -
    m[ 8 ] * m[ 2 ] * m[ 5 ];

  float det = m[ 0 ] * inv[ 0 ] + m[ 1 ] * inv[ 4 ] + m[ 2 ] * inv[ 8 ] + m[ 3 ] * inv[ 12 ];

  if( det == 0 )
  {
    *resultExists = false;
    return;
  }

  inv /= det;
  *result = inv;
  *resultExists = true;
}
m4 M4TransformInverse( v3 scale, v3 eulerRads, v3 translate )
{
  v3 s(
    1.0f / scale[ 0 ],
    1.0f / scale[ 1 ],
    1.0f / scale[ 2 ] );
  v3 t = -translate;

  v3 zero = {};
  if( eulerRads == zero )
  {
    m4 result = m4(
      s[ 0 ], 0, 0, s[ 0 ] * t[ 0 ],
      0, s[ 1 ], 0, s[ 1 ] * t[ 1 ],
      0, 0, s[ 2 ], s[ 2 ] * t[ 2 ],
      0, 0, 0, 1 );
    return result;
  }
  else
  {
    // NOTE( N8 ): this MUST be opposite order of Matrix4::Transform
    // ( Transform goes zyx, so TransformInverse goes xyz )
    m3 r = M3RotRadInv( eulerRads );
    float m03 = s[ 0 ] * ( t[ 0 ] * r( 0, 0 ) + t[ 1 ] * r( 0, 1 ) + t[ 2 ] * r( 0, 2 ) );
    float m13 = s[ 1 ] * ( t[ 0 ] * r( 1, 0 ) + t[ 1 ] * r( 1, 1 ) + t[ 2 ] * r( 1, 2 ) );
    float m23 = s[ 2 ] * ( t[ 0 ] * r( 2, 0 ) + t[ 1 ] * r( 2, 1 ) + t[ 2 ] * r( 2, 2 ) );

    m4 result = m4(
      s[ 0 ] * r( 0, 0 ), s[ 0 ] * r( 0, 1 ), s[ 0 ] * r( 0, 2 ), m03,
      s[ 1 ] * r( 1, 0 ), s[ 1 ] * r( 1, 1 ), s[ 1 ] * r( 1, 2 ), m13,
      s[ 2 ] * r( 2, 0 ), s[ 2 ] * r( 2, 1 ), s[ 2 ] * r( 2, 2 ), m23,
      0, 0, 0, 1 );
    return result;
  }
}
m4 M4Scale( v3 scale )
{
  return {
    scale[ 0 ], 0, 0, 0,
    0, scale[ 1 ], 0, 0,
    0, 0, scale[ 2 ], 0,
    0, 0, 0, 1 };
}
m4 M4Scale( float x, float y, float z )
{
  return M4Scale( v3( x, y, z ) );
}

m4 M4View( v3 camPos, v3 camViewDir, v3 camR, v3 camU )
{
  v3 negZ = -camViewDir;

  m4 worldToCamRot = {
    camR[ 0 ], camR[ 1 ], camR[ 2 ], 0,
    camU[ 0 ], camU[ 1 ], camU[ 2 ], 0,
    negZ[ 0 ], negZ[ 1 ], negZ[ 2 ], 0,
    0, 0, 0, 1 };

  m4 worldToCamTra = {
    1, 0, 0, -camPos[ 0 ],
    0, 1, 0, -camPos[ 1 ],
    0, 0, 1, -camPos[ 2 ],
    0, 0, 0, 1 };

  m4 result = worldToCamRot * worldToCamTra;
  return result;
}
m4 M4ViewInv( v3 camPos, v3 camViewDir, v3 camR, v3 camU )
{
  v3 negZ = -camViewDir;

  m4 camToWorRot = {
    camR[ 0 ], camU[ 0 ], negZ[ 0 ], 0,
    camR[ 1 ], camU[ 1 ], negZ[ 1 ], 0,
    camR[ 2 ], camU[ 2 ], negZ[ 2 ], 0,
    0, 0, 0, 1 };

  m4 camToWorTra = {
    1, 0, 0, camPos[ 0 ],
    0, 1, 0, camPos[ 1 ],
    0, 0, 1, camPos[ 2 ],
    0, 0, 0, 1 };

  auto result = camToWorTra * camToWorRot;
  return result;
}
m4 M4ProjPerspective( float A, float B, float mFieldOfViewYRad, float mAspectRatio )
{

  //                                                        [ x y z ] 
  //                              [x'y'z']             +-----+
  //                                            +-----/      |
  //                                     +-----/             |
  //                              +-----/                    |
  //                       +-----/     |                     x or y
  //                +-----/          x' or y'                |
  //         +-----/   \               |                     |
  //  +-----/     theta |              |                     |
  // /---------------------------------+---------------------+
  //
  // <-------------------------- z -------------------------->


  // Note( N8 ):           
  // P' (PROJ)  PROJ                         P( CAM )
  //           [sX * x / -z] = / w = [sX * x] = [sx 0  0 0][x]
  //           [sY * y / -z] = / w = [sY * y] = [0  sy 0 0][y]
  //           [(az+b) / -z] = / w = [az + b] = [0  0  a b][z]
  //           [1          ] = / w = [-z    ] = [0  0 -1 0][1]
  //
  // sX = d / (pW / 2) = cot(theta) / aspectRatio
  // sY = d / (pH / 2)

  float theta = mFieldOfViewYRad / 2.0f;
  float cotTheta = 1.0f / std::tan( theta );

  // sX, sY map to -1, 1
  float sX = cotTheta / mAspectRatio;
  float sY = cotTheta;


  return {
    sX, 0, 0, 0,
    0, sY, 0, 0,
    0, 0, A, B,
    0, 0, -1, 0 };
}
m4 M4ProjPerspectiveInv( float A, float B, float mFieldOfViewYRad, float mAspectRatio )
{
  // http://allenchou.net/2014/02/game-math-how-to-eyeball-the-inverse-of-a-matrix/
  float theta = mFieldOfViewYRad / 2.0f;
  float cotTheta = 1.0f / std::tan( theta );
  float sX = cotTheta / mAspectRatio; // maps x to -1, 1
  float sY = cotTheta; // maps y to -1, 1

  return {
    1.0f / sX, 0, 0, 0,
    0, 1.0f / sY, 0, 0,
    0, 0, 0, -1,
    0, 0, 1.0f / B, A / B };
}

}
