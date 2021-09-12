// Useful math functions

#pragma once

#include "src/common/math/tacVector3.h"

namespace Tac
{
  template< typename T > T Lerp( T from, T to, float t ) { return from + t * ( to - from ); }
  template< typename T > T Max( T a, T b )               { return a > b ? a : b; }
  template< typename T > T Min( T a, T b )               { return a < b ? a : b; }
  float                    Clamp( float x, float xMin, float xMax );
  float                    Saturate( float value );
  float                    RandomFloat0To1();
  float                    RandomFloatMinus1To1();
  float                    RandomFloatBetween( float a, float b );
  int                      RandomIndex( int );
  float                    EaseInOutQuart( float t );
  float                    Pow( float base, float exp );
  int                      RoundUpToNearestMultiple( int numToRound, int multiple );
  void                     Spring( float* posCurrent,
                                   float* velCurrent,
                                   float posTarget,
                                   float springyness,
                                   float deltaTimeSeconds );
  inline float             Abs( float value )          { return value > 0 ? value : value * -1; }
  inline float             Square( float value )       { return value * value; }
  inline float             Cube( float value )         { return value * value * value; }
  inline float             DegreesToRadians( float d ) { return d * ( 3.14f / 180.0f ); }
  inline float             RadiansToDegrees( float r ) { return r / ( 3.14f / 180.0f ); }

  //                       theta [0, pi]. if theta = 0, cartesian = { 0, 1, 0 }
  v3                       SphericalToCartesian( float radius, float theta, float phi );
  v3                       SphericalToCartesian( v3 );
  v3                       CartesianToSpherical( float, float, float );
  v3                       CartesianToSpherical( v3 );
}

