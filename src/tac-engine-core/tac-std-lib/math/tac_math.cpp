#include "tac_math.h" // self-inc

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector3i.h"

import std; // cstdlib, algorithm, cmath

// -------------
// Interpolation
// -------------

Tac::v2                       Tac::Min( const v2& a, const v2& b )
{
  return {
    Min( a.x, b.x ),
    Min( a.y, b.y ) };
}

Tac::v2                       Tac::Max( const v2& a, const v2& b )
{
  return {
    Max( a.x, b.x ),
    Max( a.y, b.y ) };
}

Tac::v2i                      Tac::Min( const v2i& a, const v2i& b )
{
  return {
    Min( a.x, b.x ),
    Min( a.y, b.y ) };
}

Tac::v2i                      Tac::Max( const v2i& a, const v2i& b )
{
  return {
    Max( a.x, b.x ),
    Max( a.y, b.y ) };
}

Tac::v3                       Tac::Min( const v3& a, const v3& b )
{
  return {
    Min( a.x, b.x ),
    Min( a.y, b.y ),
    Min( a.z, b.z ) };
}

Tac::v3                       Tac::Max( const v3& a, const v3& b )
{
  return {
    Max( a.x, b.x ),
    Max( a.y, b.y ),
    Max( a.z, b.z ) };
}

Tac::v3i                      Tac::Min( const v3i& a, const v3i& b )
{
  return {
    Min( a.x, b.x ),
    Min( a.y, b.y ),
    Min( a.z, b.z ),
  };
}

Tac::v3i                      Tac::Max( const v3i& a, const v3i& b )
{
  return {
    Max( a.x, b.x ),
    Max( a.y, b.y ),
    Max( a.z, b.z ),
  };
}


float Tac::EaseInOutQuart( float t )
{
  if( t < 0.5f )
    return 8 * t * t * t * t;
  t -= 1;
  return 1 - 8 * t * t * t * t;
}


void Tac::Spring( float* posCurrent,
             float* velCurrent,
             float posTarget,
             float springyness,
             float deltaTimeSeconds )
{
  // http://allenchou.net/2015/04/game-math-precise-control-over-numeric-springing/
  const float x { *posCurrent };
  const float v { *velCurrent };
  const float f { 1.0f + 2.0f * deltaTimeSeconds * springyness };
  const float oo { springyness * springyness };
  const float hoo { deltaTimeSeconds * oo };
  const float hhoo { deltaTimeSeconds * hoo };
  const float detInv { 1.0f / ( f + hhoo ) };
  const float detX { f * x + deltaTimeSeconds * v + hhoo * posTarget };
  const float detV { v + hoo * ( posTarget - x ) };
  *posCurrent = detX * detInv;
  *velCurrent = detV * detInv;
}



float Tac::Saturate( const float value )
{
  return Clamp( value, 0.0f, 1.0f );
}

float Tac::Clamp( float x, float xMin, float xMax )
{
  if( x < xMin )
    return xMin;

  if( x > xMax )
    return xMax;

  return x;
}

// ------------
// Trigonometry
// ------------

float  Tac::Sin( float f )  { return std::sinf( f ); }
double Tac::Sin( double d ) { return std::sin( d );  }
float  Tac::Cos( float f )  { return std::cosf( f ); }
double Tac::Cos( double d ) { return std::cos( d );  }
float  Tac::Tan( float f )  { return std::tanf( f ); }
double Tac::Tan( double d ) { return std::tan( d );  }

float Tac::Atan2( float y, float x ) { return std::atan2( y, x ); }
float Tac::Asin( float f ) { return std::asinf( f ); }
float Tac::Acos( float f ) { return std::acosf( f ); }

// --------
// Rounding
// --------

float Tac::Round( float f ) { return std::roundf( f ); }

// Example:
//   RoundUpToNearestMultiple( 10, 5 ) = 10
//   RoundUpToNearestMultiple( 10, 4 ) = 12
int Tac::RoundUpToNearestMultiple( int numToRound, int multiple )
{
  return ( ( numToRound + ( multiple - 1 ) ) / multiple ) * multiple;
}

float                    Tac::Floor( float f )
{
  return std::floor( f );
}

Tac::v2                       Tac::Floor( const v2& v )
{
  return {
    Floor( v.x ),
    Floor( v.y ) };
}

Tac::v3                       Tac::Floor( const v3& v )
{
  return {
    Floor( v.x ),
    Floor( v.y ),
    Floor( v.z ) };
}

float                    Tac::Ceil( float f )
{
  return std::ceil( f );
}

Tac::v2                       Tac::Ceil( const v2& v )
{
  return {
    Ceil( v.x ),
    Ceil( v.y ) };
}

Tac::v3                       Tac::Ceil( const v3& v )
{
  return {
    Ceil( v.x ),
    Ceil( v.y ),
    Ceil( v.z ) };
}

float Tac::Fmod( float x, float y )
{
  return std::fmodf( x, y );
}

double Tac::Fmod( double x, double y )
{
  return std::fmod( x, y );
}

// ----------
// Randomness
// ----------


static std::mt19937 mersenneTwister;

float Tac::RandomFloat0To1()
{
  return ( float )mersenneTwister() / mersenneTwister.max();
}

int Tac::RandomIndex( int n )
{
  return mersenneTwister() % n;
}

float Tac::RandomFloatMinus1To1()
{
  return ( RandomFloat0To1() * 2.0f ) - 1.0f;
}

float Tac::RandomFloatBetween( float a, float b )
{
  return Lerp( a, b, RandomFloat0To1() );
}
// ---------------------
// Spherical Coordinates
// ---------------------

Tac::v3 Tac::CartesianToSpherical( const v3& v ) { return CartesianToSpherical( v.x, v.y, v.z ); }

Tac::v3 Tac::SphericalToCartesian( const float r, const float t, const float p ) // radius, theta, phi
{
  const float x { r * Cos( p ) * Sin( t ) };
  const float y { r * Cos( t ) };
  const float z { r * Sin( p ) * Sin( t ) };
  return v3( x, y, z );
}

Tac::v3 Tac::SphericalToCartesian( const v3& v ) { return SphericalToCartesian( v.x, v.y, v.z ); }

Tac::v3 Tac::CartesianToSpherical( const float x, const float y, const float z )
{
  const float q { x * x + y * y + z * z };
  if( q < 0.01f )
    return {};
  const float r { Sqrt( q ) };
  const float t { Acos( y / r ) };
  const float p { Atan2( z, x ) };
  return v3( r, t, p );
}

// ------------
// Intersection
// ------------

float Tac::RaySphere( const v3& rayPos, const v3& rayDir, const v3& spherePos, float sphereRadius )
{
  const v3 dp { rayPos - spherePos };
  const float c { Dot( dp, dp ) - Square( sphereRadius ) };
  const float b { 2 * Dot( dp, rayDir ) };
  const float a { Dot( rayDir, rayDir ) };
  const float discriminant { Square( b ) - 4 * a * c };
  if( discriminant < 0 )
    return -1;
  const float rt { Sqrt( discriminant ) };
  for( float sign : { -1.0f, 1.0f } )
  {
    const float t { ( -b + sign * rt ) / ( 2 * a ) };
    if( t > 0 )
      return t;
  }
  return -1;
}

Tac::v3 Tac::ClosestPointLineSegment( const v3& p0, const v3& p1, const v3& p )
{
  const v3 to1 { p1 - p0 };
  const v3 toP { p - p0 };
  const float toP_dot_to1 { Dot( toP, to1 ) };
  if( toP_dot_to1 < 0 )
    return p0;

  const float q { Dot( to1, to1 ) };
  if( q < 0.001f )
    return p0;

  const float t { toP_dot_to1 / q };
  if( t > 1 - 0.001f )
    return p1;

  return p0 + to1 * t;
}


void Tac::ClosestPointTwoLineSegments( const v3& p1, // line 1 begin
                                  const v3& q1, // line 1 end
                                  const v3& p2, // line 2 begin
                                  const v3& q2, // line 2 end
                                  v3* c1, // closest point on line 1
                                  v3* c2 ) // closest point on line 2
{
  // this function ripped from real time collision detection
  float s { -1 };
  float t { -1 };
  const float EPSILON { 0.001f };
  v3 d1 { q1 - p1 };
  v3 d2 { q2 - p2 };
  v3 r { p1 - p2 };
  float a { Dot( d1, d1 ) };
  float e { Dot( d2, d2 ) };
  float f { Dot( d2, r ) };
  if( a <= EPSILON && e <= EPSILON )
  {
    s = 0;
    t = 0;
  }
  else if( a <= EPSILON )
  {
    s = 0;
    t = f / e;
    t = Saturate( t);
  }
  else if( e <= EPSILON )
  {
    float c = Dot( d1, r );
    t = 0;
    s = Saturate( -c / a );
  }
  else
  {
    float c { Dot( d1, r ) };
    float b { Dot( d1, d2 ) };
    float denom { a * e - b * b };
    s = denom ? Saturate( ( b * f - c * e ) / denom ) : 0;
    t = ( b * s + f ) / e;
    if( t < 0 )
    {
      t = 0;
      s = Saturate( -c / a);
    }
    else if( t > 1 )
    {
      t = 1;
      s = Saturate( ( b - c ) / a);
    }
  }
  *c1 = p1 + d1 * s;
  *c2 = p2 + d2 * t;
}

// ------------
// Arithmetic
// ------------

float Tac::Fract( float f )
{
  float integralpart;
  float fractionalpart { std::modf( f, &integralpart ) };
  return fractionalpart;
}

Tac::v2 Tac::Abs( const v2& v )
{
  return { Abs( v.x ), Abs( v.y ) };
}

Tac::v3 Tac::Abs( const v3& v )
{
  return { Abs( v.x ), Abs( v.y ), Abs( v.z ) };
}

float Tac::Pow( float base, float exp )
{
  return std::pow( base, exp );
}

float Tac::Sqrt( float f ) { return std::sqrtf( f ); }

float Tac::Exp( float f ) { return std::exp( f ); }

// -----------------
// Misc?? / Unsorted
// -----------------

bool                     Tac::IsNan( float f )
{
  return std::isnan( f );
}

bool                     Tac::IsInf( float f )
{
  return std::isinf( f );
}
