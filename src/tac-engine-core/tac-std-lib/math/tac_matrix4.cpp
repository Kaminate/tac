#include "tac_matrix4.h" // self-inc

#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_math.h"

namespace Tac
{
  struct AB
  {
    AB( m4::ProjectionMatrixParams params )
    {
      // Given the following knowns:
      //   -n in view space is projected onto parmas.mNDCNearZ in ndc space
      //   -f in view space is projected onto parmas.mNDCFarZ in ndc space
      //   
      // We are solving for the following unknowns:
      //   A
      //   B
      //
      // [ sX 0  0  0 ] [ x_view ] = [ sX * x_view    ]           [ ( sX * x_view ) / -z_view    ]
      // [ 0  sY 0  0 ] [ y_view ] = [ sY * y_view    ] /w_clip = [ ( sY * y_view ) / -z_view    ]
      // [ 0  0  A  B ] [ z_view ] = [ z_view * A + B ]           [ ( z_view * A + B ) / -z_view ]
      // [ 0  0 -1  0 ] [ 1      ] = [    -z_view     ]           [              1               ]
      //    ProjMtx     view space      clip space                          ndc space
      //
      // z_ndc = ( ( z_view ) A + B ) / ( -z_view )
      //
      // Two equations, two unknowns
      // params.mNDCMinZ = ( -nA + B ) / n
      // params.mNDCMaxZ = ( -fA + B ) / f

      const float f { params.mViewSpaceFar };
      const float n { params.mViewSpaceNear };

      const float ndcZMin { params.mNDCMinZ }; // 0 in DirectX, -1 in OpenGL
      const float ndcZMax { params.mNDCMaxZ }; // 1 in DirectX, 1 in OpenGL

      float A {};
      float B {};

      if( ndcZMin == -1 && ndcZMax == 1 ) // OpenGL style
      {
        // -1 = ( -nA + B ) / n
        //  1 = ( -fA + B ) / f
        A = ( n + f ) / ( n - f );
        B = n * ( A - 1 );
      }
      else if(  ndcZMin == 0 && ndcZMax == 1 ) // DirectX style
      {
        // 0 = ( -nA + B ) / n
        // 1 = ( -fA + B ) / f
        A = ( 1 * f ) / ( n - f );
        B = ( n * f ) / ( n - f );

        // note that https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
        // uses -A and -B from ours, but their perspective matrix uses a 1 instead of our -1
      }
      else
      {
        TAC_ASSERT_INVALID_CODE_PATH;
      }

      mA = A;
      mB = B;
    }

    float mA;
    float mB;
  };

  m4::m4( const m3& m )
  {
    *this = m4( m.m00, m.m01, m.m02, 0,
                m.m10, m.m11, m.m12, 0,
                m.m20, m.m21, m.m22, 0,
                0, 0, 0, 1 );
  }

  m4::m4( float f )
    : m00( f ), m01( 0 ), m02( 0 ), m03( 0 )
    , m10( 0 ), m11( f ), m12( 0 ), m13( 0 )
    , m20( 0 ), m21( 0 ), m22( f ), m23( 0 )
    , m30( 0 ), m31( 0 ), m32( 0 ), m33( f ) {}

  m4::m4( float mm00, float mm01, float mm02, float mm03,
          float mm10, float mm11, float mm12, float mm13,
          float mm20, float mm21, float mm22, float mm23,
          float mm30, float mm31, float mm32, float mm33 )
    : m00( mm00 ), m01( mm01 ), m02( mm02 ), m03( mm03 )
    , m10( mm10 ), m11( mm11 ), m12( mm12 ), m13( mm13 )
    , m20( mm20 ), m21( mm21 ), m22( mm22 ), m23( mm23 )
    , m30( mm30 ), m31( mm31 ), m32( mm32 ), m33( mm33 ) {}

  auto m4::data() dynmc -> dynmc float*                           { return &m00; }
  auto m4::data() const -> const float*                           { return &m00; }
  void m4::Transpose()
  {
    *this = m4( m00, m10, m20, m30,
                m01, m11, m21, m31,
                m02, m12, m22, m32,
                m03, m13, m23, m33 );
  }

  auto m4::GetRow( int r ) const -> v4
  {
    return *( ( v4* )data() + r );
  }

  auto m4::GetColumn( int c ) const -> v4
  {
    return { data()[ 4 * 0 + c ],
             data()[ 4 * 1 + c ],
             data()[ 4 * 2 + c ],
             data()[ 4 * 3 + c ] };
  }

  void m4::SetRow( int r, const v4& v )
  {
    ( ( v4* )data() )[ r ] = v;
  }

  void m4::SetColumn( int c, const v4&  v )
  {
    for( int i{}; i < 4; ++i )
      this->operator()( i, c ) = v[ i ];
  }

  auto m4::Determinant() const -> float
  {
    const float det00{ m3( m11, m12, m13,
                           m21, m22, m23,
                           m31, m32, m33 ).Determinant() };
    const float det01{ m3( m10, m12, m13,
                           m20, m22, m23,
                           m30, m32, m33 ).Determinant() };
    const float det02{ m3( m10, m11, m13,
                           m20, m21, m23,
                           m30, m31, m33 ).Determinant() };
    const float det03{ m3( m10, m11, m12,
                           m20, m21, m22,
                           m30, m31, m32 ).Determinant() };
    return det00 - det01 + det02 - det03;
  }

  auto m4::operator()( int iRow, int iCol ) dynmc -> dynmc float& { return data()[ 4 * iRow + iCol ]; }
  auto m4::operator()( int iRow, int iCol ) const -> const float& { return data()[ 4 * iRow + iCol ]; }
  auto m4::operator[]( int i ) dynmc -> dynmc float&              { return data()[ i ]; }
  auto m4::operator[]( int i ) const -> const float&              { return data()[ i ]; }
  void m4::operator /= ( const float f )
  {
    const float finv = 1.0f / f;
    for( int i{}; i < 16; ++i )
      data()[ i ] *= finv;
  }
  bool m4::operator == ( const m4& m ) const
  {
    for( int i{}; i < 16; ++i )
      if( data()[ i ] != m[ i ] )
        return false;
    return true;
  }

  m4::operator m3() const
  {
    // returns the top left 3x3 matrix
    return {
      m00, m01, m02,
      m10, m11, m12,
      m20, m21, m22,
    };
  }


  m4 m4::FromRows( const v4& r0, const v4& r1, const v4& r2, const v4& r3 )
  {
    return { r0.x, r0.y, r0.z, r0.w,
             r1.x, r1.y, r1.z, r1.w,
             r2.x, r2.y, r2.z, r2.w,
             r3.x, r3.y, r3.z, r3.w };
  }

  m4 m4::FromColumns( const v4& c0, const v4& c1, const v4& c2, const v4& c3 )
  {
    return { c0.x, c1.x, c2.x, c3.x,
             c0.y, c1.y, c2.y, c3.y,
             c0.z, c1.z, c2.z, c3.z,
             c0.w, c1.w, c2.w, c3.w };
  }

  m4 m4::Identity()
  {
    return { 1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1 };
  }

  m4 m4::Scale( const v3& v ) { return m3::Scale( v ); }
  m4 m4::RotRadX( float v )   { return m3::RotRadX( v ); }
  m4 m4::RotRadY( float v )   { return m3::RotRadY( v ); }
  m4 m4::RotRadZ( float v )   { return m3::RotRadZ( v ); }

  m4 m4::Transpose( m4 m )
  {
    m.Transpose();
    return m;
  }

  m4 m4::Translate( const v3& translate )
  {
    return { 1, 0, 0, translate[ 0 ],
             0, 1, 0, translate[ 1 ],
             0, 0, 1, translate[ 2 ],
             0, 0, 0, 1 };
  }

  m4 m4::Transform( const v3& scale, const m3& rot, const v3& translate )
  {
    float m00 { scale[ 0 ] * rot( 0, 0 ) };
    float m01 { scale[ 1 ] * rot( 0, 1 ) };
    float m02 { scale[ 2 ] * rot( 0, 2 ) };

    float m10 { scale[ 0 ] * rot( 1, 0 ) };
    float m11 { scale[ 1 ] * rot( 1, 1 ) };
    float m12 { scale[ 2 ] * rot( 1, 2 ) };

    float m20 { scale[ 0 ] * rot( 2, 0 ) };
    float m21 { scale[ 1 ] * rot( 2, 1 ) };
    float m22 { scale[ 2 ] * rot( 2, 2 ) };

    return m4( m00, m01, m02, translate[ 0 ],
               m10, m11, m12, translate[ 1 ],
               m20, m21, m22, translate[ 2 ],
               0, 0, 0, 1 );
  }

  m4 m4::Transform( const v3& scale, const v3& eulerRads, const v3& translate )
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

  m4 m4::TransformInverse( const v3& scale, const v3& eulerRads, const v3& translate )
  {
    const v3 s( 1.0f / scale[ 0 ],
                1.0f / scale[ 1 ],
                1.0f / scale[ 2 ] );
    const v3 t { -translate };
    const v3 zero  {} ;
    if( eulerRads == zero )
    {
      const m4 result( s[ 0 ], 0, 0, s[ 0 ] * t[ 0 ],
                       0, s[ 1 ], 0, s[ 1 ] * t[ 1 ],
                       0, 0, s[ 2 ], s[ 2 ] * t[ 2 ],
                       0, 0, 0, 1 );
      return result;
    }
    else
    {
      // NOTE( N8 ): this MUST be opposite order of Matrix4::Transform
      // ( Transform goes zyx, so TransformInverse goes xyz )
      const m3 r { m3::RotRadEulerInv( eulerRads ) };
      const float m03 { s[ 0 ] * ( t[ 0 ] * r( 0, 0 ) + t[ 1 ] * r( 0, 1 ) + t[ 2 ] * r( 0, 2 ) ) };
      const float m13 { s[ 1 ] * ( t[ 0 ] * r( 1, 0 ) + t[ 1 ] * r( 1, 1 ) + t[ 2 ] * r( 1, 2 ) ) };
      const float m23 { s[ 2 ] * ( t[ 0 ] * r( 2, 0 ) + t[ 1 ] * r( 2, 1 ) + t[ 2 ] * r( 2, 2 ) ) };
      const m4 result( s[ 0 ] * r( 0, 0 ), s[ 0 ] * r( 0, 1 ), s[ 0 ] * r( 0, 2 ), m03,
                       s[ 1 ] * r( 1, 0 ), s[ 1 ] * r( 1, 1 ), s[ 1 ] * r( 1, 2 ), m13,
                       s[ 2 ] * r( 2, 0 ), s[ 2 ] * r( 2, 1 ), s[ 2 ] * r( 2, 2 ), m23,
                       0, 0, 0, 1 );
      return result;
    }
  }

  m4 m4::View( const v3& camPos, const v3& camViewDir, const v3& camR, const v3& camU )
  {
    const v3 negZ { -camViewDir };
    const m4 worldToCamRot( camR[ 0 ], camR[ 1 ], camR[ 2 ], 0,
                            camU[ 0 ], camU[ 1 ], camU[ 2 ], 0,
                            negZ[ 0 ], negZ[ 1 ], negZ[ 2 ], 0,
                            0, 0, 0, 1 );
    const m4 worldToCamTra( 1, 0, 0, -camPos[ 0 ],
                            0, 1, 0, -camPos[ 1 ],
                            0, 0, 1, -camPos[ 2 ],
                            0, 0, 0, 1 );
    const m4 result { worldToCamRot * worldToCamTra };
    return result;
  }

  m4 m4::ViewInv( const v3& camPos, const v3& camViewDir, const v3& camR, const v3& camU )
  {
    const v3 negZ { -camViewDir };
    const m4 camToWorRot( camR[ 0 ], camU[ 0 ], negZ[ 0 ], 0,
                          camR[ 1 ], camU[ 1 ], negZ[ 1 ], 0,
                          camR[ 2 ], camU[ 2 ], negZ[ 2 ], 0,
                          0, 0, 0, 1 );
    const m4 camToWorTra( 1, 0, 0, camPos[ 0 ],
                          0, 1, 0, camPos[ 1 ],
                          0, 0, 1, camPos[ 2 ],
                          0, 0, 0, 1 );
    const m4 result { camToWorTra * camToWorRot };
    return result;
  }

  m4 m4::ProjPerspective( ProjectionMatrixParams params )
  {
    TAC_ASSERT( params.mViewSpaceNear > 0 );
    TAC_ASSERT( params.mViewSpaceFar > 0 );


    const AB ab( params );
    const float A { ab.mA};
    const float B { ab.mB };
    const float cotTheta { 1.0f / Tan( params.mFOVYRadians / 2.0f ) };
    const float sX { cotTheta / params.mAspectRatio }; // sX, sY map to -1, 1
    const float sY { cotTheta };

    return { sX, 0, 0, 0,
             0, sY, 0, 0,
             0, 0, A, B,
             0, 0, -1, 0 };
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
    const float theta { mFieldOfViewYRad / 2.0f };
    const float cotTheta { 1.0f / Tan( theta ) };
    const float sX { cotTheta / mAspectRatio }; // sX, sY map to -1, 1
    const float sY { cotTheta };
    return { sX, 0, 0, 0,
             0, sY, 0, 0,
             0, 0, A, B,
             0, 0, -1, 0 };
  }

  m4 m4::ProjPerspectiveInv( ProjectionMatrixParams params )
  {
    // http://allenchou.net/2014/02/game-math-how-to-eyeball-the-inverse-of-a-matrix/
    const float theta { params.mFOVYRadians / 2.0f };
    const float cotTheta { 1.0f / Tan( theta ) };
    const float sX { cotTheta / params.mAspectRatio }; // maps x to -1, 1
    const float sY { cotTheta }; // maps y to -1, 1
    const AB ab( params );
    const float A { ab.mA};
    const float B { ab.mB };
    return { 1.0f / sX, 0, 0, 0,
             0, 1.0f / sY, 0, 0,
             0, 0, 0, -1,
             0, 0, 1.0f / B, A / B };
  }

  m4 m4::ProjPerspectiveInv( float A, float B, float mFieldOfViewYRad, float mAspectRatio )
  {
    // http://allenchou.net/2014/02/game-math-how-to-eyeball-the-inverse-of-a-matrix/
    const float theta { mFieldOfViewYRad / 2.0f };
    const float cotTheta { 1.0f / Tan( theta ) };
    const float sX { cotTheta / mAspectRatio }; // maps x to -1, 1
    const float sY { cotTheta }; // maps y to -1, 1
    return { 1.0f / sX, 0, 0, 0,
             0, 1.0f / sY, 0, 0,
             0, 0, 0, -1,
             0, 0, 1.0f / B, A / B };
  }

} // namespace Tac

auto Tac::operator*( const m4& m, const v4& v ) -> v4
{
  v4 result;
  for( int i{}; i < 4; ++i )
  {
    float sum {};
    for( int j {}; j < 4; ++j )
    {
      sum += m( i, j ) * v[ j ];
    }
    result[ i ] = sum;
  }
  return result;
}

auto Tac::operator*( const m4& lhs, const m4& rhs ) -> m4
  {
    m4 result;
    for( int r {}; r < 4; ++r )
    {
      for( int c {}; c < 4; ++c )
      {
        float sum {};
        for( int i{}; i < 4; ++i )
        {
          sum += lhs( r, i ) * rhs( i, c );
        }
        result( r, c ) = sum;
      }
    }
    return result;
  }

