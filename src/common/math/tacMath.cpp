
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

}

