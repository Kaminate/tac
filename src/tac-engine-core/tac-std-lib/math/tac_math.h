#pragma once

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector3i.h"

namespace Tac
{

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
  double                   Tan( double );
  float                    Asin( float );
  float                    Acos( float );
  float                    Atan2( float y, float x );

  // --------
  // Rounding
  // --------
  float                    Round( float );
  constexpr int            RoundUpToNearestMultiple( int numToRound, int multiple )
  {
    return ( ( numToRound + ( multiple - 1 ) ) / multiple ) * multiple;
  }
  float                    Floor( float );
  double                   Floor( double );
  v2                       Floor( const v2& );
  v3                       Floor( const v3& );
  float                    Ceil( float );
  v2                       Ceil( const v2& );
  v3                       Ceil( const v3& );
  float                    Fmod( float, float );
  double                   Fmod( double, double );

  // ----------
  // Randomness
  // ----------

  float                    RandomFloat0To1();
  float                    RandomFloatMinus1To1();
  float                    RandomFloatBetween( float, float );
  int                      RandomIndex( int n ); // [ 0, n )
  v3                       RandomPointInTriangle( v3, v3, v3 );

  // ---------------------
  // Angles
  // ---------------------

  template< typename T > T DegreesToRadians( T d ) { return d * ( 3.14f / 180.0f ); }
  template< typename T > T RadiansToDegrees( T r ) { return r * ( 180.0f / 3.14f ); }

  //            y        (x,y,z)
  //            ^           +
  //            |          /|
  //            |<theta>__/ |
  //            |    __/    |
  //            | __/       |
  //            |/          |
  //            +------^----|---+---> x
  //           / \___ phi   |  /
  //          /      \_v_   | /
  //         /        r  \__|/
  //        z               +        
  struct SphericalCoordinate
  {
    v3          ToCartesian() const;
    static auto FromCartesian( v3 ) -> SphericalCoordinate;

    float mRadius { 1 };
    float mTheta  {}; // [0, pi]
    float mPhi    {}; // [0, 2pi]
  };

  // --------
  // Geometry
  // --------
  struct Ray3D{ v3 mOrigin; v3 mDirection; };
  struct Triangle3D
  {
    v3 mVertices[ 3 ];
    auto operator[]( int i ) -> v3& { return mVertices[ i ]; }
    auto operator[]( int i ) const -> const v3& { return mVertices[ i ]; }
  };
  struct LineSegment3D
  {
    v3 mEndPoints[ 2 ];
    auto operator[]( int i ) -> v3& { return mEndPoints[ i ]; }
    auto operator[]( int i ) const -> const v3& { return mEndPoints[ i ]; }
  };
  struct Sphere3D { v3 mOrigin; float mRadius; };

  using Ray = Ray3D;
  using Triangle = Triangle3D;
  using LineSegment = LineSegment3D;
  using Sphere = Sphere3D;

  // ------------
  // Intersection
  // ------------

  auto RaySphere( Ray, Sphere ) -> float; // returns -1 on failure
  auto ClosestPointLineSegment( LineSegment, const v3& p ) -> v3;

  struct RayTriangle
  {
    RayTriangle( const Ray&, const Triangle& );
    auto GetIntersectionPoint( const Triangle& ) const -> v3;
    float mT{};
    float mU{};
    float mV{};
    bool  mValid{};
  };

  struct ClosestPointLineSegments
  {
    struct Input { LineSegment mLine1; LineSegment mLine2; };
    ClosestPointLineSegments( Input );
    v3 mClosestPointOnLine1;
    v3 mClosestPointOnLine2;
  };
  
  struct ClosestPointTwoRays
  {
    struct Input { Ray mRay0; Ray mRay1; };
    ClosestPointTwoRays( Input );
    float mRay0T;
    float mRay1T;
  };

  // ------------
  // Arithmetic
  // ------------
  float                    Fract( float );
  inline float             Abs( float v )              { return v > 0 ? v : -v; }
  inline double            Abs( double v )             { return v > 0 ? v : -v; }
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

  bool IsNan( float );
  bool IsInf( float );

  auto SampleCosineWeightedHemisphere() -> SphericalCoordinate;
  auto SampleCosineWeightedHemisphere(v3 n) -> v3;

} // namespace Tac

