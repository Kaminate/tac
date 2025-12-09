#include "tac_vector4.h"

#include "tac-std-lib/math/tac_math.h"

namespace Tac
{
  v4::v4( float val )                              : x( val ), y( val ), z( val ), w( val ) { }
  v4::v4( float xx, float yy, float zz, float ww ) : x( xx ), y( yy ), z( zz ), w( ww ) {}
  v4::v4( const v3& xyz, float ww )                : x( xyz.x ), y( xyz.y ), z( xyz.z ), w( ww ) {}
  v4::v4( const v2& xy, float zz, float ww )       : x( xy.x ), y( xy.y ), z( zz ), w( ww ) {}
  auto v4::xy() dynmc -> dynmc v2&                   { return ( v2& )x; }
  auto v4::xy() const -> const v2&                   { return ( v2& )x; }
  auto v4::xyz() dynmc -> dynmc v3&                  { return ( v3& )x; }
  auto v4::xyz() const -> const v3&                  { return ( v3& )x; }
  auto v4::begin() -> float*                         { return data(); }
  auto v4::end() -> float*                           { return data() + 4; }
  auto v4::data() dynmc -> dynmc float*              { return &x; }
  auto v4::data() const -> const float*              { return &x; }
  auto v4::operator[]( int i ) dynmc -> dynmc float& { return data()[ i ]; }
  auto v4::operator[]( int i ) const -> const float& { return data()[ i ]; }
  void v4::operator /= ( float v )                   { *this *= ( 1.0f / v ); }
  void v4::operator *= ( float v )                   { for( int i{}; i < 4; ++i ) data()[ i ] *= v; }
  void v4::operator -= ( const v4& v )               { for( int i{}; i < 4; ++i ) data()[ i ] -= v[ i ]; }
  void v4::operator += ( const v4& v )               { for( int i{}; i < 4; ++i ) data()[ i ] += v[ i ]; }
  bool v4::operator == ( const v4& v ) const         { return x == v.x && y == v.y && z == v.z && w == v.w; }
  bool v4::operator != ( const v4& v ) const         { return x != v.x || y != v.y || z != v.z || w != v.w; }
  auto v4::operator - () const -> v4                 { return *this * -1.0f; }
  auto v4::operator * ( float v ) const -> v4        { v4 res = *this; res *= v; return res; }
  auto v4::operator / ( float v ) const -> v4        { return *this * ( 1.0f / v ); }
  auto v4::operator + ( const v4& v ) const -> v4    { v4 res = *this; res += v; return res; }
  auto v4::operator - ( const v4& v ) const -> v4    { v4 res = *this; res -= v; return res; }
  void v4::Normalize()                               { *this /= Length(); }
  auto v4::Length() const -> float                   { return Sqrt( Quadrance() ); }
  auto v4::Quadrance() const -> float                { return dot( *this, *this ); }
}

auto Tac::operator *( float f, const v4& v ) -> v4      { return v * f; }
auto Tac::dot( const v4& lhs, const v4& rhs ) -> float  { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w; }
auto Tac::Distance( const v4& l, const v4& r ) -> float { return ( l - r ).Length(); }
