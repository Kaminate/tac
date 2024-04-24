#include "tac-std-lib/math/tac_matrix2.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_math_unit_test_helper.h"
#include "tac-std-lib/algorithm/tac_algorithm.h" // Swap

namespace Tac
{
  float* m2::data()
  {
    return &m00;
  }

  const float* m2::data() const
  {
    return &m00;
  }

  m2::m2( float mm00, float mm01,
          float mm10, float mm11)
    : m00( mm00 ), m01( mm01 )
    , m10( mm10 ), m11( mm11 ) {}

  float m2::determinant() const
  {
    return m00 * m11 - m01 * m10;
  }

  float& m2::operator()( int iRow, int iCol )
  {
    return data()[ 2 * iRow + iCol ];
  }

  float m2::operator()( int iRow, int iCol ) const
  {
    return data()[ 2 * iRow + iCol ];
  }

  float& m2::operator[]( int i )
  {
    return data()[ i ];
  }

  float m2::operator[]( int i ) const
  {
    return data()[ i ];
  }

  m2&          m2::operator*= (float f)
  {
    for( int i{}; i < 4; ++i )
      data()[i] *= f;

    return *this;
  }

  void         m2::Transpose()
  {
    Swap( m01, m10 );
  }

  m2 m2::Identity()
  {
    return {
      1, 0,
      0, 1 };
  }
  void m2::Invert(bool* valid, m2* m) const
  {
    const float d = determinant();
    if( !( *valid = d != 0 ) )
      return;

    *m = m2(
      m11, -m01,
      -m10, m00
    ) * ( 1.0f / d );
  }

  m2 operator* (float f, const m2& m)
  {
    m2 r = m;
    return r *= f;
  }

  m2 operator* (const m2& m, float f)
  {
    m2 r = m;
    return r *= f;
  }

  v2 operator* ( const m2& m, const v2& v )
  {
    return { m.m00 * v.x + m.m01 * v.y,
             m.m10 * v.x + m.m11 * v.y, };
  }

  void AssertAboutEqual( const m2& a, const m2& b )
  {
    AssertAboutEqual(a.data(), b.data(), 2 * 2 );
  }

  void m2UnitTestInverse( m2 m, m2 minvexpected)
  {
    m2 minvfound{};
    bool b;
    m.Invert(&b, &minvfound);
    AssertAboutEqual(minvfound, minvexpected);
  }

  void m2UnitTestInverse()
  {
    m2 m{ 3, 3.2f,
          3.5f, 3.6f };
    m2 minv{ -9, 8, 
      8.75f, -7.5f };

    m2UnitTestInverse(m, minv);
  }

  void m2UnitTest()
  {
    m2UnitTestInverse();
  }
}
