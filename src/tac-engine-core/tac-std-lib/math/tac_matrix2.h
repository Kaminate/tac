#pragma once

namespace Tac
{
  struct v2;
  struct m2
  {
    float
      m00, m01,
      m10, m11;

    m2() = default;
    m2( float mm00, float mm01,
        float mm10, float mm11 );
    float*       data();
    const float* data() const;
    float        determinant() const;
    float&       operator()( int iRow, int iCol );
    float        operator()( int iRow, int iCol ) const;
    float&       operator[]( int );
    float        operator[]( int ) const;
    m2&          operator*= (float);
    void         Transpose();
    void         Invert(bool*, m2*) const;
    static m2    Identity();
  };

  m2 operator* (float, const m2& );
  m2 operator* (const m2&, float );
  v2 operator* (const m2&, const v2& );

  void m2UnitTest();
} // namespace Tac

