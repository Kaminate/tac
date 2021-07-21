#pragma once

#include "src/common/math/tacVector4.h"

namespace Tac
{
  struct v3;
  struct v4;
  struct m3;
  struct m4
  {
    float
      m00, m01, m02, m03,
      m10, m11, m12, m13,
      m20, m21, m22, m23,
      m30, m31, m32, m33;
    m4() = default;
    m4( const m3& );
    m4( float, float, float, float,
        float, float, float, float,
        float, float, float, float,
        float, float, float, float );
    float*       data();
    const float* data() const;
    float&       operator()( int iRow, int iCol );
    float        operator()( int iRow, int iCol ) const;
    float&       operator[]( int );
    float        operator[]( int ) const;
    bool         operator== ( const m4& ) const;
    void         operator/= ( float );
    void         Transpose();
    v4           GetRow( int );
    v4           GetColumn( int );
    void         SetRow( int, v4 );
    void         SetColumn( int, v4 );

    static m4    FromRows( v4, v4, v4, v4 );
    static m4    FromColumns( v4, v4, v4, v4 );
    static m4    Identity();
    static m4    Inverse( const m4&, bool* resultExists );
    static m4    Scale( v3 );
    static m4    RotRadX( float );
    static m4    RotRadY( float );
    static m4    RotRadZ( float );
    static m4    Translate( v3 translate );
    static m4    Transform( v3 scale, m3 rot, v3 translate );
    static m4    Transform( v3 scale, v3 eulerRads, v3 translate );
    static m4    TransformInverse( v3 scale, v3 eulerRads, v3 translate );
    static m4    View( v3 camPos, v3 camViewDir, v3 camR, v3 camU );
    static m4    ViewInv( v3 camPos, v3 camViewDir, v3 camR, v3 camU );
    static m4    ProjPerspective( float A, float B, float mFieldOfViewYRad, float mAspectRatio );
    static m4    ProjPerspectiveInv( float A, float B, float mFieldOfViewYRad, float mAspectRatio );
  };

  v4 operator * ( const m4&, const v4& );
  m4 operator * ( const m4&, const m4& );
}

