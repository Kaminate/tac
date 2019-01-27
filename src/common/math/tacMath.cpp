#include "common/math/tacMath.h"
#include <cstdlib>
#include <algorithm>

float TacEaseInOutQuart( float t )
{
  if( t < 0.5f )
    return 8 * t * t * t * t;
  t -= 1;
  return 1 - 8 * t * t * t * t;
}


void TacSpring(
  float* posCurrent,
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

float TacRandomFloat0To1()
{
  return ( float )std::rand() / RAND_MAX;
}
float TacRandomFloatMinus1To1()
{
  return ( TacRandomFloat0To1() * 2.0f ) - 1.0f;
}

float TacRandomFloatBetween( float a, float b )
{
  return TacLerp( a, b, TacRandomFloat0To1() );
}

int TacRoundUpToNearestMultiple( int numToRound, int multiple )
{
  return ( ( numToRound + ( multiple - 1 ) ) / multiple ) * multiple;
}
