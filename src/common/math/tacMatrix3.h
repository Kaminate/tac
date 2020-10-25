#pragma once

namespace Tac
{
  struct v2;
  struct v3;
  struct m3
  {
    float
      m00, m01, m02,
      m10, m11, m12,
      m20, m21, m22;
    m3() = default;
    m3( float mm00, float mm01, float mm02,
        float mm10, float mm11, float mm12,
        float mm20, float mm21, float mm22 );
    float*       data();
    const float* data() const;
    float        determinant() const;
    float&       operator()( int iRow, int iCol );
    float        operator()( int iRow, int iCol ) const;
    float&       operator[]( int );
    float        operator[]( int ) const;
    static m3    FromColumns( const v3&, const v3&, const v3& );
    static m3    FromRows( const v3&, const v3&, const v3& );
    static m3    Identity();
    static m3    Scale( const v3& );
    static m3    RotRadEuler( const v3& );
    static m3    RotRadEulerInv( const v3& );
    static m3    RotRadX( float );
    static m3    RotRadY( float );
    static m3    RotRadZ( float );
    static m3    RotRadAngleAxis( float, const v3& );
    static m3    Translate( const v2& );
  };

  v3 operator*( const m3&, const v3& );
  m3 operator*( const m3&, const m3& );
}

