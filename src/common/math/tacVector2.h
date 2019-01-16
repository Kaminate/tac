#pragma once

struct v2
{
  float x;
  float y;
  float* begin();
  float* end();
  float* data();
  const float* data() const;
  v2() = default;
  v2( float xx, float yy );
  float& operator[]( int i );
  float operator[]( int i ) const;
  void operator /= ( float v );
  void operator *= ( float v );
  void operator -= ( const v2& v );
  void operator += ( const v2& v );
  bool operator == ( const v2& v )const;
  bool operator != ( const v2& v )const;
  v2 operator - () const;
  v2 operator * ( float v ) const;
  v2 operator / ( float v ) const;
  v2 operator + ( const v2& v ) const;
  v2 operator - ( const v2& v ) const;
  void Normalize();
  float Length() const;
  float Quadrance() const;
};

v2 operator *( float f, const v2& v );
float TacDot( const v2& lhs, const v2& rhs );
v2 Normalize( const v2& v );
float Length( const v2& v );
float Distance( const v2& lhs, const v2& rhs );
float TacQuadrance( const v2& v );
float TacQuadrance( const v2& lhs, const v2& rhs );
