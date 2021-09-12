
#include "src/common/math/tacMath.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

namespace Tac
{
  float Pow( float base, float exp )
  {
    return std::pow( base, exp );
  }

  float EaseInOutQuart( float t )
  {
    if( t < 0.5f )
      return 8 * t * t * t * t;
    t -= 1;
    return 1 - 8 * t * t * t * t;
  }


  void Spring( float* posCurrent,
    float* velCurrent,
    float posTarget,
    float springyness,
    float deltaTimeSeconds )
  {
    // http://allenchou.net/2015/04/game-math-precise-control-over-numeric-springing/
    float x = *posCurrent;
    float v = *velCurrent;
    float f = 1.0f + 2.0f * deltaTimeSeconds * springyness;
    float oo = springyness * springyness;
    float hoo = deltaTimeSeconds * oo;
    float hhoo = deltaTimeSeconds * hoo;
    float detInv = 1.0f / ( f + hhoo );
    float detX = f * x + deltaTimeSeconds * v + hhoo * posTarget;
    float detV = v + hoo * ( posTarget - x );
    *posCurrent = detX * detInv;
    *velCurrent = detV * detInv;
  }

  float RandomFloat0To1()
  {
    return ( float )std::rand() / RAND_MAX;
  }

  int RandomIndex( int n )
  {
    return std::rand() % n;
  }

  float RandomFloatMinus1To1()
  {
    return ( RandomFloat0To1() * 2.0f ) - 1.0f;
  }

  float RandomFloatBetween( float a, float b )
  {
    return Lerp( a, b, RandomFloat0To1() );
  }

  int RoundUpToNearestMultiple( int numToRound, int multiple )
  {
    return ( ( numToRound + ( multiple - 1 ) ) / multiple ) * multiple;
  }

  float Saturate( float value )
  {
    return Clamp( value, 0.0f, 1.0f );
  }
  float Clamp( float x, float xMin, float xMax )
  {
    if( x < xMin )
      return xMin;
    if( x > xMax )
      return xMax;
    return x;
  }

  v3 SphericalToCartesian( const float r, const float t, const float p ) // radius, theta, phi
  {
    const float x = r * std::cos( p ) * std::sin( t );
    const float y = r * std::cos( t );
    const float z = r * std::sin( p ) * std::sin( t );
    return v3( x, y, z );
  }

  v3 SphericalToCartesian( const v3 v ) { return SphericalToCartesian( v.x, v.y, v.z ); }

  v3 CartesianToSpherical( const float x, const float y, const float z )
  {
    const float q = x * x + y * y + z * z;
    if( q < 0.01f )
      return {};
    const float r = std::sqrt( q );
    const float t = std::acos( y / r );
    const float p = std::atan2( z, x );
    return v3( r, t, p );
  }

  v3 CartesianToSpherical( const v3 v ) { return CartesianToSpherical( v.x, v.y, v.z ); }
}

