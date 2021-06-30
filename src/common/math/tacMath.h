// Useful math functions

#pragma once
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
}

