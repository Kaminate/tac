
// Useful math functions

#pragma once
namespace Tac
{
  template< typename T > T Lerp( T from, T to, float t ) { return from + t * ( to - from ); }
  template< typename T > T Max( T a, T b ) { return a > b ? a : b; }
  template< typename T > T Min( T a, T b ) { return a < b ? a : b; }
  float Clamp( float x, float xMin, float xMax );
  float Saturate( float value );
  float RandomFloat0To1();
  float RandomFloatMinus1To1();
  float RandomFloatBetween( float a, float b );
  float EaseInOutQuart( float t );
  float Pow( float base, float exp );
  int RoundUpToNearestMultiple( int numToRound, int multiple );
  void Spring( float* posCurrent,
               float* velCurrent,
               float posTarget,
               float springyness,
               float deltaTimeSeconds );

}

