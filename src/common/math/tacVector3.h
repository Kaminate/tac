#pragma once

#include "src/common/math/tacVector2.h"

namespace Tac
{


struct v3
{
  float x; //= 0;
  float y; // = 0;
  float z; // = 0;
  float* begin();
  float* end();
  float* data();
  const float* data() const;
  v3() = default;
  v3( v2 xy, float z);
  v3( float xx, float yy, float zz );
  v2& xy();
  float& operator[]( int i );
  float operator[]( int i ) const;
  void operator /= ( float v );
  void operator *= ( float v );
  void operator -= ( const v3& v );
  void operator += ( const v3& v );
  bool operator == ( const v3& v ) const;
  bool operator != ( const v3& v ) const;
  v3 operator - () const;
  v3 operator * ( float v ) const;
  v3 operator / ( float v ) const;
  v3 operator + ( const v3& v ) const;
  v3 operator - ( const v3& v ) const;
  void Normalize();
  float Length() const;
  float Quadrance() const;
};

v3 operator *( float f, const v3& v );
float Dot( const v3& lhs, const v3& rhs );
v3 Cross( const v3& lhs, const v3& rhs );
v3 Normalize( const v3& v );
float Length( const v3& v );
float Distance( const v3& lhs, const v3& rhs );
float Quadrance( const v3& v );
float Quadrance( const v3& lhs, const v3& rhs );

// float Angle( v3 a, v3 b ) { return std::acos( Dot( a, b ) / ( Length( a ) * Length( b ) ) ); }

v3 Project( const v3& onto_b, const v3& of_a );

void GetFrameRH( const v3& normalizedDir, v3& unittan1, v3& unittan2 );
}
