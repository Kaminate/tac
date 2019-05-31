#pragma once
#include "tacVector4.h"
#include "tacMatrix3.h"

struct m4
{
  float mValues[ 16 ];// = {};
  float* data();
  const float* data() const;
  m4() = default;
  m4( const m3& m );
  m4(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33 );
  float& operator()( int iRow, int iCol );
  float operator()( int iRow, int iCol ) const;
  float& operator[]( int i );
  float operator[]( int i ) const;
  bool operator == ( const m4& m ) const;
  void operator /= ( float f );
  int getValueIndex( int iRow, int iCol ) const;
  static m4 Identity();
};

v4 operator * ( const m4& m, const v4& v );
m4 operator * ( const m4& , const m4&  );

m4 M4Scale( v3 scale );
m4 M4Scale( float x, float y, float z );
m4 M4Transform( v3 scale, m3 rot, v3 translate );
m4 M4Transform( v3 scale, v3 eulerRads, v3 translate );
m4 M4TransformInverse( v3 scale, v3 eulerRads, v3 translate );
void M4Inverse( const m4& m, m4* result, bool* resultExists );
m4 M4Translate( v3 translate );
m4 M4Translate( float x, float y, float z );
m4 M4RotRadX( float rotRad );
m4 M4RotRadY( float rotRad );
m4 M4RotRadZ( float rotRad );


m4 M4View( v3 camPos, v3 camViewDir, v3 camR, v3 camU );
m4 M4ViewInv( v3 camPos, v3 camViewDir, v3 camR, v3 camU );
m4 M4ProjPerspective( float A, float B, float mFieldOfViewYRad, float mAspectRatio );
m4 M4ProjPerspectiveInv( float A, float B, float mFieldOfViewYRad, float mAspectRatio );



