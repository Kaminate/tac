#include "tac_vector3.h"

#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_math_unit_test_helper.h"

namespace Tac
{
  v3::v3( float xx, float yy, float zz ) : x( xx ), y( yy ), z( zz ){}
  v3::v3( float xyz ) : x( xyz ), y( xyz ), z( xyz ){}
  v3::v3( v2 xy, float zz ) : x( xy.x ), y( xy.y ), z( zz ){}
  auto v3::xy() const -> const v2&                   { return *( v2* )data(); }
  auto v3::xy() dynmc -> dynmc v2&                   { return *( v2* )data(); }
  auto v3::begin() -> float*                         { return data(); }
  auto v3::end() -> float*                           { return data() + 3; }
  auto v3::data() dynmc -> dynmc float*              { return &x; }
  auto v3::data() const -> const float*              { return &x; }
  void v3::Normalize()                               { *this /= Length(); }
  auto v3::Length() const -> float                   { return Sqrt( Quadrance() ); }
  auto v3::Quadrance() const -> float                { return Dot( *this, *this ); }
  auto v3::operator[]( int i ) dynmc -> dynmc float& { return data()[ i ]; }
  auto v3::operator[]( int i ) const -> const float& { return data()[ i ]; }
  void v3::operator /= ( float v )                   { *this *= ( 1.0f / v ); }
  void v3::operator *= ( float v )                   { for( int i{}; i < 3; ++i ) data()[ i ] *= v; }
  void v3::operator -= ( const v3& v )               { for( int i{}; i < 3; ++i ) data()[ i ] -= v[ i ]; }
  void v3::operator += ( const v3& v )               { for( int i{}; i < 3; ++i ) data()[ i ] += v[ i ]; }
  bool v3::operator == ( const v3& v ) const         { return x == v.x && y == v.y && z == v.z; }
  bool v3::operator != ( const v3& v ) const         { return x != v.x || y != v.y || z != v.z; }
  auto v3::operator - () const -> v3                 { return *this * -1.0f; }
  auto v3::operator * ( float v ) const -> v3        { v3 result{ *this }; result *= v; return result; }
  auto v3::operator / ( float v ) const -> v3        { return *this * ( 1.0f / v ); }
  auto v3::operator + ( const v3& v ) const -> v3    { v3 result{ *this }; result += v; return result; }
  auto v3::operator - ( const v3& v ) const -> v3    { v3 result{ *this }; result -= v; return result; }
} // namespace Tac

auto Tac::operator *( float f, const v3& v ) -> v3           { return v * f; }
auto Tac::Dot( const v3& lhs, const v3& rhs ) -> float       { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }
auto Tac::Cross( const v3& l, const v3& r ) -> v3            { return v3( l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x ); }
auto Tac::Normalize( const v3& v ) -> v3                     { v3 result { v }; result.Normalize(); return result; }
auto Tac::Length( const v3& v ) -> float                     { return v.Length(); }
auto Tac::Distance( const v3& lhs, const v3& rhs ) -> float  { return ( lhs - rhs ).Length(); }
auto Tac::Quadrance( const v3& v ) -> float                  { return v.Quadrance(); }
auto Tac::Quadrance( const v3& lhs, const v3& rhs ) -> float { return ( lhs - rhs ).Quadrance(); }
 
auto Tac::Project( const v3& onto_b, const v3& of_a ) -> v3
{
  const float b_lengthsq { Quadrance( onto_b ) };
  if( b_lengthsq < 0.0001f )
    return onto_b;

  const v3 onto_b_dir { onto_b / Sqrt( b_lengthsq ) };
  const v3 result { onto_b_dir * Dot( onto_b_dir, of_a ) };
  return result;
}

void Tac::GetFrameRH( const v3& unitDir, v3& unitTan1, v3& unitTan2 )
{
  // From Erin Catto's blog post "Computing a Basis" posted on February 3, 2014

  // Suppose vector a has all equal components and is a unit vector:
  // Length( a ) = s^2 + s^2 + s^2 = 1
  // Then 3*s^2 = 1, s = sqrt(1/3) = 0.57735.
  // This means that at least one component of a unit vector must be
  // greater or equal to 0.57735.

  if( Abs( unitDir[ 0 ] ) >= 0.57735f )
  {
    unitTan1[ 0 ] = unitDir[ 1 ];
    unitTan1[ 1 ] = -unitDir[ 0 ];
    unitTan1[ 2 ] = 0.0f;
  }
  else
  {
    unitTan1[ 0 ] = 0.0f;
    unitTan1[ 1 ] = unitDir[ 2 ];
    unitTan1[ 2 ] = -unitDir[ 1 ];
  }

  unitTan1.Normalize();
  unitTan2 = Cross( unitDir, unitTan1 );
}

void Tac::v3UnitTest()
{
  v3 a{ 3,-3,1 };
  v3 b{ 4,9,2 };
  v3 acrossb = { -15, -2,39 };
  v3 c = Cross( a, b );
  AssertAboutEqual( c, acrossb );
}

