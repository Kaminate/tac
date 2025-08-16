#include "tac_matrix3.h" // self-inc

#include "tac-std-lib/math/tac_matrix2.h"
#include "tac-std-lib/math/tac_math_unit_test_helper.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_math.h" // Abs
#include "tac-std-lib/algorithm/tac_algorithm.h" // Swap

//#include <cmath> // sin, cos, sqrt

namespace Tac
{

#if UseQuat()
    TmpQuat::TmpQuat(const m3& rotation)
    {
      float trace { rotation.Trace() };
      if( trace > 0.0f )
      {
        float s { Sqrt( trace + 1.0f ) };
        w = s * 0.5f;
        float recip { 0.5f / s };
        x = ( rotation( 2, 1 ) - rotation( 1, 2 ) ) * recip;
        y = ( rotation( 0, 2 ) - rotation( 2, 0 ) ) * recip;
        z = ( rotation( 1, 0 ) - rotation( 0, 1 ) ) * recip;
      }
      else
      {
        unsigned int i{};
        if( rotation( 1, 1 ) > rotation( 0, 0 ) )
          i = 1;
        if( rotation( 2, 2 ) > rotation( i, i ) )
          i = 2;
        unsigned int j { ( i + 1 ) % 3 };
        unsigned int k { ( j + 1 ) % 3 };
        float s { Sqrt( rotation( i, i ) - rotation( j, j ) - rotation( k, k ) + 1.0f ) };
        ( *this )[ i ] = 0.5f * s;
        float recip { 0.5f / s };
        w = ( rotation( k, j ) - rotation( j, k ) ) * recip;
        ( *this )[ j ] = ( rotation( j, i ) + rotation( i, j ) ) * recip;
        ( *this )[ k ] = ( rotation( k, i ) + rotation( i, k ) ) * recip;
      }
    }

    void TmpQuat::Zero()
    {
      w = 0;
      x = 0;
      y = 0;
      z = 0;
    }
    void TmpQuat::Normalize()
    {
      float lengthsq { w * w + x * x + y * y + z * z };

      if( Abs(lengthsq) < 0.001f)
      {
        Zero();
      }
      else
      {
        float factor { 1.0f / Sqrt( lengthsq ) };
        w *= factor;
        x *= factor;
        y *= factor;
        z *= factor;
      }

    }

    float& TmpQuat::operator[]( int i )         { return ( &x )[ i ]; }
    float TmpQuat::operator[]( int i ) const    { return ( &x )[ i ]; }

    m3::m3(TmpQuat rotate )
    {
      float xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

      xs = rotate.x + rotate.x;
      ys = rotate.y + rotate.y;
      zs = rotate.z + rotate.z;
      wx = rotate.w * xs;
      wy = rotate.w * ys;
      wz = rotate.w * zs;
      xx = rotate.x * xs;
      xy = rotate.x * ys;
      xz = rotate.x * zs;
      yy = rotate.y * ys;
      yz = rotate.y * zs;
      zz = rotate.z * zs;

      float mV[9];
      mV[ 0 ] = 1.0f - ( yy + zz );
      mV[ 3 ] = xy - wz;
      mV[ 6 ] = xz + wy;

      mV[ 1 ] = xy + wz;
      mV[ 4 ] = 1.0f - ( xx + zz );
      mV[ 7 ] = yz - wx;

      mV[ 2 ] = xz - wy;
      mV[ 5 ] = yz + wx;
      mV[ 8 ] = 1.0f - ( xx + yy );
      *this = m3(
        mV[0], mV[3], mV[6],
        mV[1], mV[4], mV[7],
        mV[2], mV[5], mV[8] );
    }
#endif

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

  float m3::Determinant() const
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
    for( int r {}; r < 3; ++r )
    {
      float sum {};
      for( int c {}; c < 3; ++c )
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
    for( int r {}; r < 3; ++r )
    {
      for( int c {}; c < 3; ++c )
      {
        float sum {};
        for( int i{}; i < 3; ++i )
        {
          sum += lhs( r, i ) * rhs( i, c );
        }
        result( r, c ) = sum;
      }
    }
    return result;
  }

  float           m3::Trace() const
  {
    return m00 + m11 + m22;
  }

  v3           m3::GetColumn( int i ) const
  {
    switch( i )
    {
    case 0: return { m00, m10, m20 };
    case 1: return { m01, m11, m21 };
    case 2: return { m02, m12, m22 };
    default: TAC_ASSERT_INVALID_CASE(i); return {};
    }
  }

  v3           m3::GetRow( int i ) const
  {
    switch( i )
    {
    case 0: return { m00, m01, m02 };
    case 1: return { m10, m11, m12 };
    case 2: return { m20 , m21 , m22 };
    default: TAC_ASSERT_INVALID_CASE(i); return {};
    }
  }

  bool m3::Invert(m3* m) const
  {
    float d { Determinant() };
    if( d == 0 )
      return false;
    m3 a { Adjugate() };
    *m = ( 1 / d ) * a;
    return true;
  }

  m3           m3::Adjugate() const
  {
    m3 result { Cofactor() };
    result.Transpose();
    return result;
  }

  m3           m3::Cofactor() const
  {
      float c00 = m2( m11, m12,
                      m21, m22 ).Determinant();
      float c01 = m2( m10, m12,
                      m20, m22 ).Determinant();
      float c02 = m2( m10, m11,
                      m20, m21 ).Determinant();
      float c10 = m2( m01, m02,
                      m21, m22 ).Determinant();
      float c11 = m2( m00, m02,
                      m20, m22 ).Determinant();
      float c12 = m2( m00, m01,
                      m20, m21 ).Determinant();
      float c20 = m2( m01, m02,
                      m11, m12 ).Determinant();
      float c21 = m2( m00, m02,
                      m10, m12 ).Determinant();
      float c22 = m2( m00, m01,
                      m10, m11 ).Determinant();
      return { c00, -c01, c02,
               -c10, c11, -c12,
               c20, -c21, c22 };
  }

  void         m3::Transpose()
  {
    Swap( m01, m10 );
    Swap( m02, m20 );
    Swap( m12, m21 );
  }

  // Ortho normalizes a matrix
  static void GramSchmidt( m3& m )
  {
    v3 x { m.GetColumn( 0 ) };
    x.Normalize();

    v3 y { m.GetColumn( 1 ) };
    y -= Project( x, y );
    y.Normalize();

    v3 z { m.GetColumn( 2 ) };
    z -= Project( x, z );
    z -= Project( y, z );
    z.Normalize();

    m = m3::FromColumns( x, y, z );
  }

  void         m3::OrthoNormalize()
  {
    GramSchmidt( *this );
  }

  m3 m3::Transpose( const m3& m )
  {
    return  { m.m00, m.m10, m.m20,
              m.m01, m.m11, m.m21,
              m.m02, m.m12, m.m22 };
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
    float s { Sin( rotRad ) };
    float c { Cos( rotRad ) };
    return {
      1, 0, 0,
      0, c, -s,
      0, s, c };
  }

  m3 m3::RotRadY( float rotRad )
  {
    float s { Sin( rotRad ) };
    float c { Cos( rotRad ) };
    return {
      c, 0, s,
      0, 1, 0,
      -s, 0, c };
  }

  m3 m3::RotRadZ( float rotRad )
  {
    float s { Sin( rotRad ) };
    float c { Cos( rotRad ) };
    return {
      c, -s, 0,
      s, c, 0,
      0, 0, 1 };
  }

  m3 m3::RotRadAngleAxis( const float angleRadians, const v3& unitAxis )
  {
    const float c { Cos( angleRadians ) };
    const float s { Sin( angleRadians ) };
    const float t { 1.0f - c };
    const float m00 { c + unitAxis[ 0 ] * unitAxis[ 0 ] * t };
    const float m11 { c + unitAxis[ 1 ] * unitAxis[ 1 ] * t };
    const float m22 { c + unitAxis[ 2 ] * unitAxis[ 2 ] * t };
    const float m10 { unitAxis[ 0 ] * unitAxis[ 1 ] * t + unitAxis[ 2 ] * s };
    const float m01 { unitAxis[ 0 ] * unitAxis[ 1 ] * t - unitAxis[ 2 ] * s };
    const float m20 { unitAxis[ 0 ] * unitAxis[ 2 ] * t - unitAxis[ 1 ] * s };
    const float m02 { unitAxis[ 0 ] * unitAxis[ 2 ] * t + unitAxis[ 1 ] * s };
    const float m21 { unitAxis[ 1 ] * unitAxis[ 2 ] * t + unitAxis[ 0 ] * s };
    const float m12 { unitAxis[ 1 ] * unitAxis[ 2 ] * t - unitAxis[ 0 ] * s };
    return
    {
      m00, m01, m02,
      m10, m11, m12,
      m20, m21, m22
    };
  }

  m3 m3::Scale( float x, float y, float z )
  {
    return {
      x, 0, 0,
      0, y, 0,
      0, 0, z };
  }

  m3 m3::Scale( const v3& scale )
  {
    return {
      scale[ 0 ], 0, 0,
      0, scale[ 1 ], 0,
      0, 0, scale[ 2 ] };
  }

  m3 m3::Translate( float x, float y )
  {
    return {
      1, 0, x,
      0, 1, y,
      0, 0, 1 };
  }
  m3 m3::Translate( const v2& v )
  {
    return {
      1, 0, v.x,
      0, 1, v.y,
      0, 0, 1 };
  }

  m3    m3::CrossProduct( float x, float y, float z )
  {
    return { 0, -z, y,
             z, 0, -x,
            -y, x, 0 };
  }

  m3    m3::CrossProduct( const v3& v )
  {
    return CrossProduct(v.x, v.y, v.z );
  }

  void m3::operator*= ( float f )
  {
    for( int i{}; i < 3 * 3; ++i )
      data()[ i ] *= f;
  }

  void m3::operator+= ( const m3& m )
  {
    float* dst { this->data() };
    const float* src { m.data() };
    for( int i{}; i < 3 * 3; ++i )
    {
      dst[ i ] += src[ i ];
    }
  }

  m3 operator* (float f, const m3& m)
  {
    m3 result { m };
    result *=f;
    return result;
  }


  static void m3UnitTestInverse(m3 m, m3 mInvExpected)
  {
    m3 mInv;
    bool invExists { m.Invert(&mInv) };
    TAC_ASSERT( invExists );
    AssertAboutEqual( mInv, mInvExpected );
  }

  static void m3UnitTestInverse()
  {
    m3 m{ 0, 2, 2,
          1, 1, 2,
          2, 1, 2 };
    m3 minvExpected{ 0, -1, 1,
                     1, -2, 1,
                     -0.5f, 2, -1 };
    m3UnitTestInverse( m, minvExpected );
  }

  static void m3UnitTestMultiply()
  {
    m3 a{ 1, 2, 3,
          4, 5, 6,
          7, 8, 9 };
    m3 b{ 11, 22, 33,
          44, 55, 66,
          77, 88, 99 };
    m3 c = a * b;
    m3 cExpected{ 330, 396, 462,
                  726, 891, 1056,
                  1122, 1386, 1650 };
    AssertAboutEqual( c, cExpected );
  }

  void m3UnitTest()
  {
    m3UnitTestInverse();

    m3UnitTestMultiply();
  }
}
