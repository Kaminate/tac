#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_math.h"
//#include <cmath> // sqrt

namespace Tac
{
  v2::v2( float xx, float yy ) : x( xx ), y( yy ) {}
  auto v2::begin() -> float*                          { return data(); }
  auto v2::end() -> float*                            { return data() + 2; }
  auto v2::data() -> float*                           { return &x; }
  auto v2::data() const -> const float*               { return &x; }
  void v2::Normalize()                                { *this /= Length(); }
  auto v2::Length() const    -> float                 { return Sqrt( Quadrance() ); }
  auto v2::Quadrance() const -> float                 { return Dot( *this, *this ); }
  auto v2::operator[]( int i ) dynmc -> dynmc float&  { return data()[ i ]; }
  auto v2::operator[]( int i ) const -> const float&  { return data()[ i ]; }
  void v2::operator /= ( float v )                    { *this *= ( 1.0f / v ); }
  void v2::operator *= ( float v )                    { for( int i{}; i < 2; ++i ) data()[ i ] *= v; }
  void v2::operator -= ( const v2& v )                { for( int i{}; i < 2; ++i ) data()[ i ] -= v[ i ]; }
  void v2::operator += ( const v2& v )                { for( int i{}; i < 2; ++i ) data()[ i ] += v[ i ]; }
  bool v2::operator == ( const v2& v ) const
  {
    for( int i{}; i < 2; ++i )
      if( data()[ i ] != v[ i ] )
        return false;
    return true;
  }
  bool v2::operator != ( const v2& v ) const
  {
    for( int i{}; i < 2; ++i )
      if( data()[ i ] != v[ i ] )
        return true;
    return false;
  }
  auto v2::operator - () const -> v2                  { return *this * -1.0f; }
  auto v2::operator * ( float v ) const -> v2         { v2 result = *this; result *= v; return result; }
  auto v2::operator / ( float v ) const -> v2         { return *this * ( 1.0f / v ); }
  auto v2::operator / ( int v ) const -> v2           { return *this * ( 1.0f / v ); }
  auto v2::operator + ( const v2& v ) const -> v2     { v2 result = *this; result += v; return result; }
  auto v2::operator - ( const v2& v ) const -> v2     { v2 result = *this; result -= v; return result; }
  v2::operator v2i() const                            { return { ( int )x, ( int )y }; } // this function should probably not exist
}

auto Tac::operator *( float f, const v2& v ) -> v2       { return v * f; }
auto Tac::Dot( const v2& l, const v2& r ) -> float       { return l.x * r.x + l.y * r.y; }
auto Tac::Normalize( const v2& v ) -> v2                 { v2 result = v; result.Normalize(); return result; }
auto Tac::Length( const v2& v ) -> float                 { return v.Length(); }
auto Tac::Distance( const v2& l, const v2& r ) -> float  { return ( l - r ).Length(); }
auto Tac::Quadrance( const v2& v ) -> float              { return v.Quadrance(); }
auto Tac::Quadrance( const v2& l, const v2& r ) -> float { return ( l - r ).Quadrance(); }
