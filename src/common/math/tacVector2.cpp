#include "src/common/math/tacVector2.h"
#include <cmath> // sqrt

namespace Tac
{
  v2::v2( float xx, float yy ) : x( xx ), y( yy ) {}
  float*       v2::begin()      { return data(); }
  float*       v2::end()        { return data() + 2; }
  float*       v2::data()       { return &x; }
  const float* v2::data() const { return &x; }
  float&       v2::operator[]( int i ) { return data()[ i ]; }
  float        v2::operator[]( int i ) const { return data()[ i ]; }
  void         v2::operator /= ( float v ) { *this *= ( 1.0f / v ); }
  void         v2::operator *= ( float v ) { for( int i = 0; i < 2; ++i ) data()[ i ] *= v; }
  void         v2::operator -= ( const v2& v ) { for( int i = 0; i < 2; ++i ) data()[ i ] -= v[ i ]; }
  void         v2::operator += ( const v2& v ) { for( int i = 0; i < 2; ++i ) data()[ i ] += v[ i ]; }
  bool         v2::operator == ( const v2& v ) const
  {
    for( int i = 0; i < 2; ++i )
      if( data()[ i ] != v[ i ] )
        return false;
    return true;
  }
  bool         v2::operator != ( const v2& v ) const
  {
    for( int i = 0; i < 2; ++i )
      if( data()[ i ] != v[ i ] )
        return true;
    return false;
  }
  v2           v2::operator - () const { return *this * -1.0f; }
  v2           v2::operator * ( float v ) const { v2 result = *this; result *= v; return result; }
  v2           v2::operator / ( float v ) const { return *this * ( 1.0f / v ); }
  v2           v2::operator + ( const v2& v ) const { v2 result = *this; result += v; return result; }
  v2           v2::operator - ( const v2& v ) const { v2 result = *this; result -= v; return result; }
  void         v2::Normalize()       { *this /= Length(); }
  float        v2::Length() const    { return std::sqrt( Quadrance() ); }
  float        v2::Quadrance() const { return Dot( *this, *this ); }

  v2           operator *( float f, const v2& v ) { return v * f; }
  float        Dot( const v2& lhs, const v2& rhs )
  {
    float result = 0;
    for( int i = 0; i < 2; ++i )
      result += lhs[ i ] * rhs[ i ];
    return result;
  }
  v2           Normalize( const v2& v )                  { v2 result = v; result.Normalize(); return result; }
  float        Length( const v2& v )                     { return v.Length(); }
  float        Distance( const v2& lhs, const v2& rhs )  { return ( lhs - rhs ).Length(); }
  float        Quadrance( const v2& v )                  { return v.Quadrance(); }
  float        Quadrance( const v2& lhs, const v2& rhs ) { return ( lhs - rhs ).Quadrance(); }

}
