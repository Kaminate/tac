#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_math_unit_test_helper.h"
#include "tac-std-lib/math/tac_math.h"
//#include <cmath> // sqrt

namespace Tac
{
  v3::v3( float xx, float yy, float zz ) : x( xx ), y( yy ), z( zz ){}
  v3::v3( float xyz ) : x( xyz ), y( xyz ), z( xyz ){}
  v3::v3( v2 xy, float zz ) : x( xy.x ), y( xy.y ), z( zz ){}
  v2&          v3::xy() { return *( v2* )data(); }
  float*       v3::begin() { return data(); }
  float*       v3::end() { return data() + 3; }
  float*       v3::data() { return &x; }
  const float* v3::data() const{ return &x; }
  float&       v3::operator[]( int i ) { return data()[ i ]; }
  float        v3::operator[]( int i ) const { return data()[ i ]; }
  void         v3::operator /= ( float v ) { *this *= ( 1.0f / v ); }
  void         v3::operator *= ( float v ) { for( int i{}; i < 3; ++i ) data()[ i ] *= v; }
  void         v3::operator -= ( const v3& v ) { for( int i{}; i < 3; ++i ) data()[ i ] -= v[ i ]; }
  void         v3::operator += ( const v3& v ) { for( int i{}; i < 3; ++i ) data()[ i ] += v[ i ]; }
  bool         v3::operator == ( const v3& v )const
  {
    for( int i { 0 }; i < 3; ++i )
      if( data()[ i ] != v[ i ] )
        return false;
    return true;
  }
  bool         v3::operator != ( const v3& v )const
  {
    for( int i { 0 }; i < 3; ++i )
      if( data()[ i ] != v[ i ] )
        return true;
    return false;
  }
  v3           v3::operator - () const { return *this * -1.0f; }
  v3           v3::operator * ( float v ) const { v3 result = *this; result *= v; return result; }
  v3           v3::operator / ( float v ) const { return *this * ( 1.0f / v ); }
  v3           v3::operator + ( const v3& v ) const { v3 result = *this; result += v; return result; }
  v3           v3::operator - ( const v3& v ) const { v3 result = *this; result -= v; return result; }
  void         v3::Normalize() { *this /= Length(); }
  float        v3::Length() const { return Sqrt( Quadrance() ); }
  float        v3::Quadrance() const { return Dot( *this, *this ); }

  v3 operator *( float f, const v3& v ) { return v * f; }

  float Dot( const v3& lhs, const v3& rhs )
  {
    return
      lhs.x * rhs.x +
      lhs.y * rhs.y +
      lhs.z * rhs.z;
  }

  v3    Cross( const v3& l, const v3& r )
  {
    return{
      l.y * r.z - l.z * r.y,
      l.z * r.x - l.x * r.z,
      l.x * r.y - l.y * r.x };
  }

  v3    Normalize( const v3& v ){ v3 result = v; result.Normalize(); return result; }

  float Length( const v3& v ){ return v.Length(); }

  float Distance( const v3& lhs, const v3& rhs ) { return ( lhs - rhs ).Length(); }

  float Quadrance( const v3& v ){ return v.Quadrance(); }

  float Quadrance( const v3& lhs, const v3& rhs ){ return ( lhs - rhs ).Quadrance(); }

  v3    Project( const v3& onto_b, const v3& of_a )
  {
    const float b_lengthsq = Quadrance( onto_b );
    if( b_lengthsq < 0.0001f )
      return onto_b;

    const v3 onto_b_dir = onto_b / Sqrt( b_lengthsq );
    const v3 result = onto_b_dir * Dot( onto_b_dir, of_a );
    return result;
  }

  void  GetFrameRH( const v3& unitDir, v3& unitTan1, v3& unitTan2 )
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

  void           v3UnitTestCross( v3 a, v3 b, v3 expected )
  {
    v3 c = Cross( a, b );
    AssertAboutEqual( c, expected );
  }

  void           v3UnitTest()
  {
    v3 a{ 3,-3,1 };
    v3 b{ 4,9,2 };
    v3 acrossb = { -15, -2,39 };
    v3UnitTestCross( a, b, acrossb );
  }
} // namespace Tac

