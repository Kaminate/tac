#include "src/common/math/tacMatrix4.h"
#include "src/common/math/tacMatrix3.h"
#include "src/common/math/tacVector4.h"
#include <cmath>
#include <cstring> // memcmp

namespace Tac
{



  m4::m4( const m3& m )
  {
    *this = {
      m.m00, m.m01, m.m02, 0,
      m.m10, m.m11, m.m12, 0,
      m.m20, m.m21, m.m22, 0,
      0, 0, 0, 1 };
  }

  m4::m4( float mm00, float mm01, float mm02, float mm03,
          float mm10, float mm11, float mm12, float mm13,
          float mm20, float mm21, float mm22, float mm23,
          float mm30, float mm31, float mm32, float mm33 )
    : m00( mm00 ), m01( mm01 ), m02( mm02 ), m03( mm03 )
    , m10( mm10 ), m11( mm11 ), m12( mm12 ), m13( mm13 )
    , m20( mm20 ), m21( mm21 ), m22( mm22 ), m23( mm23 )
    , m30( mm30 ), m31( mm31 ), m32( mm32 ), m33( mm33 ) {}


  float* m4::data() { return &m00; }
  const float* m4::data() const { return &m00; }

  float& m4::operator()( int iRow, int iCol ) { return data()[ 4 * iRow + iCol ]; }

  float m4::operator()( int iRow, int iCol ) const { return data()[ 4 * iRow + iCol ]; }

  float& m4::operator[]( int i ) { return data()[ i ]; }

  float m4::operator[]( int i ) const { return data()[ i ]; }

  void m4::operator /= ( const float f )
  {
    const float finv = 1.0f / f;
    for( int i = 0; i < 16; ++i )
      data()[ i ] *= finv;
  }

  bool m4::operator == ( const m4& m ) const
  {
    for( int i = 0; i < 16; ++i )
      if( data()[ i ] != m[ i ] )
        return false;
    return true;
  }

  v4 operator*( const m4& m, const v4& v )
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

  m4 operator*( const m4& lhs, const m4& rhs )
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

  m4 m4::Identity()
  {
    return {
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1 };
  }

  m4 m4::Scale( v3 v ) { return m3::Scale( v ); }

  m4 m4::RotRadX( float v ) { return m3::RotRadX( v ); }

  m4 m4::RotRadY( float v ) { return m3::RotRadY( v ); }

  m4 m4::RotRadZ( float v ) { return m3::RotRadZ( v ); }

  m4 m4::Translate( v3 translate )
  {
    return {
      1, 0, 0, translate[ 0 ],
      0, 1, 0, translate[ 1 ],
      0, 0, 1, translate[ 2 ],
      0, 0, 0, 1 };
  }

  m4 m4::Transform( v3 scale, m3 rot, v3 translate )
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

  m4 m4::Transform( v3 scale, v3 eulerRads, v3 translate )
  {
    const m3 rot = m3::RotRadEuler( eulerRads );
    return m4::Transform( scale, rot, translate );
  }

  m4 m4::Inverse( const m4& m, bool* resultExists )
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

    const float det =
      m[ 0 ] * inv[ 0 ] +
      m[ 1 ] * inv[ 4 ] +
      m[ 2 ] * inv[ 8 ] +
      m[ 3 ] * inv[ 12 ];

    if( det == 0 )
    {
      *resultExists = false;
      return {};
    }

    inv /= det;
    *resultExists = true;
    return inv;
  }

  m4 m4::TransformInverse( v3 scale, v3 eulerRads, v3 translate )
  {
    v3 s( 1.0f / scale[ 0 ],
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
      const m3 r = m3::RotRadEulerInv( eulerRads );
      const float m03 = s[ 0 ] * ( t[ 0 ] * r( 0, 0 ) + t[ 1 ] * r( 0, 1 ) + t[ 2 ] * r( 0, 2 ) );
      const float m13 = s[ 1 ] * ( t[ 0 ] * r( 1, 0 ) + t[ 1 ] * r( 1, 1 ) + t[ 2 ] * r( 1, 2 ) );
      const float m23 = s[ 2 ] * ( t[ 0 ] * r( 2, 0 ) + t[ 1 ] * r( 2, 1 ) + t[ 2 ] * r( 2, 2 ) );

      m4 result = m4(
        s[ 0 ] * r( 0, 0 ), s[ 0 ] * r( 0, 1 ), s[ 0 ] * r( 0, 2 ), m03,
        s[ 1 ] * r( 1, 0 ), s[ 1 ] * r( 1, 1 ), s[ 1 ] * r( 1, 2 ), m13,
        s[ 2 ] * r( 2, 0 ), s[ 2 ] * r( 2, 1 ), s[ 2 ] * r( 2, 2 ), m23,
        0, 0, 0, 1 );
      return result;
    }
  }

  m4 m4::View( v3 camPos, v3 camViewDir, v3 camR, v3 camU )
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

  m4 m4::ViewInv( v3 camPos, v3 camViewDir, v3 camR, v3 camU )
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

  m4 m4::ProjPerspective( float A, float B, float mFieldOfViewYRad, float mAspectRatio )
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
    //
    //
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
    float sX = cotTheta / mAspectRatio; // sX, sY map to -1, 1
    float sY = cotTheta;
    return { sX, 0, 0, 0,
             0, sY, 0, 0,
             0, 0, A, B,
             0, 0, -1, 0 };
  }

  m4 m4::ProjPerspectiveInv( float A, float B, float mFieldOfViewYRad, float mAspectRatio )
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
