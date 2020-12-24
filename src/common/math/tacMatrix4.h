#pragma once

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
    m4( float mm00, float mm01, float mm02, float mm03,
        float mm10, float mm11, float mm12, float mm13,
        float mm20, float mm21, float mm22, float mm23,
        float mm30, float mm31, float mm32, float mm33 );
    float*       data();
    const float* data() const;
    float&       operator()( int iRow, int iCol );
    float        operator()( int iRow, int iCol ) const;
    float&       operator[]( int );
    float        operator[]( int ) const;
    bool         operator== ( const m4& ) const;
    void         operator/= ( float );
    static m4    Identity();
    static m4    Inverse( const m4& , bool* resultExists );
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

  v4 operator * ( const m4& m, const v4& v );
  m4 operator * ( const m4&, const m4& );
}

