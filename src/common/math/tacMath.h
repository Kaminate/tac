// Useful math functions

#pragma once

template< typename T > T TacLerp( T from, T to, T t ) { return from + t * ( to - from ); }


template< typename T > T TacMax( T a, T b ) { return a > b ? a : b; }
template< typename T > T TacMin( T a, T b ) { return a < b ?  a: b; }
template< typename T >
T TacClamp( T x, T xMin, T xMax )
{
  if( x < xMin )
    return xMin;
  if( x > xMax )
    return xMax;
  return x;
}
template< typename T > T TacSquare( T value ) { return value * value; }
template< typename T > T TacSaturate( T value ) { return TacClamp( value, ( T )0, ( T )1 ); }

float TacRandomFloat0To1();
float TacRandomFloatMinus1To1();

float TacEaseInOutQuart( float t );

int TacRoundUpToNearestMultiple( int numToRound, int multiple );

void TacSpring(
  float* posCurrent,
  float* velCurrent,
  float posTarget,
  float springyness,
  float deltaTimeSeconds );


