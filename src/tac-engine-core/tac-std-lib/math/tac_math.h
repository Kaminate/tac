#pragma once

namespace Tac
{
  struct v2;
  struct v2i;
  struct v3;
  struct v3i;

  // -------------
  // Interpolation
  // -------------

  template< typename T > T Lerp( T from, T to, float t ) { return from + t * ( to - from ); }
  template< typename T > T Max( const T& a, const T& b ) { return a > b ? a : b; }
  template< typename T > T Min( const T& a, const T& b ) { return a < b ? a : b; }
  v2                       Min( const v2&, const v2& );
  v2                       Max( const v2&, const v2& );
  v2i                      Min( const v2i&, const v2i& );
  v2i                      Max( const v2i&, const v2i& );
  v3                       Min( const v3&, const v3& );
  v3                       Max( const v3&, const v3& );
  v3i                      Min( const v3i&, const v3i& );
  v3i                      Max( const v3i&, const v3i& );
  float                    Clamp( float x, float xMin, float xMax );
  float                    Saturate( float );
  float                    EaseInOutQuart( float t );
  void                     Spring( float* posCurrent,
                                   float* velCurrent,
                                   float posTarget,
                                   float springyness,
                                   float deltaTimeSeconds );

  // ------------
  // Trigonometry
  // ------------
  float                    Sin( float );
  double                   Sin( double );
  float                    Cos( float );
  double                   Cos( double );
  float                    Tan( float );
  float                    Asin( float );
  float                    Acos( float );
  float                    Atan2( float y, float x );

  // --------
  // Rounding
  // --------
  float                    Round( float );
  int                      RoundUpToNearestMultiple( int numToRound, int multiple );
  float                    Floor(float);
  v2                       Floor(const v2&);
  v3                       Floor(const v3&);
  //double                   Floor(int);
  float                    Ceil(float);
  v2                       Ceil(const v2&);
  v3                       Ceil(const v3&);
  //double                   Ceil(int);

  float                    Fmod( float, float );
  double                   Fmod( double, double );

  // ----------
  // Randomness
  // ----------

  float                    RandomFloat0To1();
  float                    RandomFloatMinus1To1();
  float                    RandomFloatBetween( float, float );
  int                      RandomIndex( int n ); // [ 0, n )

  // ---------------------
  // Angles
  // ---------------------

  template< typename T > T DegreesToRadians( T d ) { return d * ( 3.14f / 180.0f ); }
  template< typename T > T RadiansToDegrees( T r ) { return r * ( 180.0f / 3.14f ); }

  // ---------------------
  // Spherical Coordinates
  // ---------------------

  //                       theta [0, pi]. if theta = 0, cartesian = { 0, 1, 0 }
  v3                       SphericalToCartesian( float radius, float theta, float phi );
  v3                       SphericalToCartesian( const v3& );
  v3                       CartesianToSpherical( float, float, float );
  v3                       CartesianToSpherical( const v3& );

  // ------------
  // Intersection
  // ------------

  //                       returns -1 on failure
  float                    RaySphere( const v3& rayPos,
                                      const v3& rayDir,
                                      const v3& spherePos,
                                      float sphereRadius );
  v3                       ClosestPointLineSegment( const v3& p0, const v3& p1, const v3& p );

  void                     ClosestPointTwoLineSegments( const v3& p1, // line 1 begin
                                                        const v3& q1, // line 1 end
                                                        const v3& p2, // line 2 begin
                                                        const v3& q2, // line 2 end
                                                        v3* c1, // closest point on line 1
                                                        v3* c2 ); // closest point on line 2
  
  // ------------
  // Arithmetic
  // ------------
  float                    Fract( float );
  inline float             Abs( float v )              { return v > 0 ? v : -v; }
  v2                       Abs( const v2& );
  v3                       Abs( const v3& );
  float                    Pow( float base, float exp );
  float                    Sqrt( float );
  float                    Exp( float ); // Exp(x) = e^x
  inline float             Square( float v )           { return v * v; }
  inline float             Cube( float v )             { return v * v * v; }

  // -----------------
  // Misc?? / Unsorted / IEEE 754 Floating Point
  // -----------------

  bool                     IsNan(float);
  bool                     IsInf(float);
}

