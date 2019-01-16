#pragma once
#include "tacVector3.h"

struct m3
{
  float
    m00, m01, m02,
    m10, m11, m12,
    m20, m21, m22;
  float* data();
  const float* data() const;
  m3() = default;
  static m3 FromColumns( const v3& c0, const v3& c1, const v3& c2 );
  static m3 FromRows( const v3& r0, const v3& r1, const v3& r2 );
  static m3 Identity();
  m3(
    float mm00, float mm01, float mm02,
    float mm10, float mm11, float mm12,
    float mm20, float mm21, float mm22 );
  float& operator()( int iRow, int iCol );
  float operator()( int iRow, int iCol ) const;
  float determinant() const;
  int getValueIndex( int iRow, int iCol ) const;
};

v3 operator * ( const m3& m, const v3& v );
m3 operator * ( const m3& , const m3&  );

float determinant( const m3& m );
m3 M3Scale( v3 scale );
m3 M3Scale( float x, float y, float z );
m3 M3Translate( float x, float y );
m3 M3Translate(v2 v);
m3 M3RotRadX( float rotRad );
m3 M3RotRadY( float rotRad );
m3 M3RotRadZ( float rotRad );
m3 M3AngleAxis( float , v3 );

