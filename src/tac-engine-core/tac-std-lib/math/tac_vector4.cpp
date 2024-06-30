#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_math.h"
//#include <cmath> // sqrt

namespace Tac
{
  v4::v4( float val )                              : x( val ), y( val ), z( val ), w( val ) { }
  v4::v4( float xx, float yy, float zz, float ww ) : x( xx ), y( yy ), z( zz ), w( ww ) {}
  v4::v4( const v3& xyz, float ww )                : x( xyz.x ), y( xyz.y ), z( xyz.z ), w( ww ) {}
  v4::v4( const v2& xy, float zz, float ww )       : x( xy.x ), y( xy.y ), z( zz ), w( ww ) {}
  v2&          v4::xy()                        { return ( v2& )x; }
  const v2&    v4::xy() const                  { return ( v2& )x; }
  v3&          v4::xyz()                       { return ( v3& )x; }
  const v3&    v4::xyz() const                 { return ( v3& )x; }
  float*       v4::begin()                     { return data(); }
  float*       v4::end()                       { return data() + 4; }
  float*       v4::data()                      { return &x; }
  const float* v4::data() const                { return &x; }
  float&       v4::operator[]( int i )         { return data()[ i ]; }
  float        v4::operator[]( int i ) const   { return data()[ i ]; }
  void         v4::operator /= ( float v )     { *this *= ( 1.0f / v ); }
  void         v4::operator *= ( float v )     { for( int i{}; i < 4; ++i ) data()[ i ] *= v; }
  void         v4::operator -= ( const v4& v ) { for( int i{}; i < 4; ++i ) data()[ i ] -= v[ i ]; }
  void         v4::operator += ( const v4& v ) { for( int i{}; i < 4; ++i ) data()[ i ] += v[ i ]; }
  bool         v4::operator == ( const v4& v ) const
  {
    return
      x == v.x &&
      y == v.y &&
      z == v.z &&
      w == v.w;
  }
  bool         v4::operator != ( const v4& v ) const
  {
    return
      x != v.x ||
      y != v.y ||
      z != v.z ||
      w != v.w;
  }
  v4           v4::operator - () const              { return *this * -1.0f; }
  v4           v4::operator * ( float v ) const     { v4 res = *this; res *= v; return res; }
  v4           v4::operator / ( float v ) const     { return *this * ( 1.0f / v ); }
  v4           v4::operator + ( const v4& v ) const { v4 res = *this; res += v; return res; }
  v4           v4::operator - ( const v4& v ) const { v4 res = *this; res -= v; return res; }
  void         v4::Normalize()                      { *this /= Length(); }
  float        v4::Length() const                   { return Sqrt( Quadrance() ); }
  float        v4::Quadrance() const                { return dot( *this, *this ); }
}

Tac::v4 Tac::operator *( float f, const v4& v )     { return v * f; }
float Tac::dot( const v4& lhs, const v4& rhs )
{
  return
    lhs.x * rhs.x +
    lhs.y * rhs.y +
    lhs.z * rhs.z +
    lhs.w * rhs.w;
}
float Tac::Distance( const v4& lhs, const v4& rhs ) { return ( lhs - rhs ).Length(); }
