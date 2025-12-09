#pragma once

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector3i.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{

  // -------------
  // Interpolation
  // -------------

  struct SpringParams { float* mPosCur; float* mVelCur; float mPosTgt; float mSpringyness; float mDeltaTimeSeconds; };
  template< typename T > auto Lerp( T from, T to, float t ) -> T { return from + t * ( to - from ); }
  template< typename T > auto Max( const T& a, const T& b ) -> T { return a > b ? a : b; }
  template< typename T > auto Min( const T& a, const T& b ) -> T { return a < b ? a : b; }
  auto Min( const v2&, const v2& ) -> v2;
  auto Max( const v2&, const v2& ) -> v2;
  auto Min( const v2i&, const v2i& ) -> v2i;
  auto Max( const v2i&, const v2i& ) -> v2i;
  auto Min( const v3&, const v3& ) -> v3;
  auto Max( const v3&, const v3& ) -> v3;
  auto Min( const v3i&, const v3i& ) -> v3i;
  auto Max( const v3i&, const v3i& ) -> v3i;
  auto Clamp( float x, float xMin, float xMax ) -> float;
  auto Saturate( float ) -> float;
  auto EaseInOutQuart( float ) -> float;
  void Spring( SpringParams );

  // ------------
  // Trigonometry
  // ------------
  auto Sin( float ) -> float;
  auto Sin( double ) -> double;
  auto Cos( float ) -> float;
  auto Cos( double ) -> double;
  auto Tan( float ) -> float;
  auto Tan( double ) -> double;
  auto Asin( float ) -> float;
  auto Acos( float ) -> float;
  auto Atan2( float y, float x ) -> float;

  // --------
  // Rounding
  // --------
  constexpr auto RoundUpToNearestMultiple( int numToRound, int multiple )
  {
    return ( ( numToRound + ( multiple - 1 ) ) / multiple ) * multiple;
  }
  auto Round( float ) -> float;
  auto Floor( float ) -> float;
  auto Floor( double ) -> double;
  auto Floor( const v2& ) -> v2;
  auto Floor( const v3& ) -> v3;
  auto Ceil( float ) -> float;
  auto Ceil( const v2& ) -> v2;
  auto Ceil( const v3& ) -> v3;
  auto Fmod( float, float ) -> float;
  auto Fmod( double, double ) -> double;

  // ----------
  // Randomness
  // ----------

  auto RandomFloat0To1() -> float;
  auto RandomFloatMinus1To1() -> float;
  auto RandomFloatBetween( float, float ) -> float;
  auto RandomIndex( int n ) -> int; // [ 0, n )
  auto RandomPointInTriangle( v3, v3, v3 ) -> v3;

  // ---------------------
  // Angles
  // ---------------------

  template< typename T > auto DegreesToRadians( T d ) -> T { return d * ( 3.14f / 180.0f ); }
  template< typename T > auto RadiansToDegrees( T r ) -> T { return r * ( 180.0f / 3.14f ); }

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
    auto ToCartesian() const -> v3;
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
  struct Plane3D
  {
    auto SignedDistance( v3 ) const -> float;
    auto UnitNormal() const -> v3;
    static auto FromPosNormal( v3 pos, v3 nor ) -> Plane3D;
    static auto FromPosUnitNormal( v3 pos, v3 nor ) -> Plane3D;
    v4 mImplicitCoeffs{}; // ax + by + cz + d = 0, and [a,b,c] is a unit normal
  };
  struct Sphere3D { v3 mOrigin; float mRadius; };

  using Ray = Ray3D;
  using Triangle = Triangle3D;
  using LineSegment = LineSegment3D;
  using Sphere = Sphere3D;
  using Plane = Plane3D;

  // ------------
  // Intersection
  // ------------

  auto RaySphere( Ray, Sphere ) -> float; // returns -1 on failure
  auto ClosestPointLineSegment( LineSegment, const v3& p ) -> v3;

  struct RayPlane
  {
    RayPlane( Ray, Plane );
    operator bool() const;
    float mT     {};
    bool  mValid {};
  };

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
  auto Fract( float ) -> float;
  auto Pow( float base, float exp ) -> float;
  auto Sqrt( float ) -> float;
  auto Exp( float ) -> float; // Exp(x) = e^x
  inline auto Abs( float v ) -> float     { return v > 0 ? v : -v; }
  inline auto Abs( double v ) -> double   { return v > 0 ? v : -v; }
  inline auto Abs( const v2& v ) -> v2    { return { Abs( v.x ), Abs( v.y ) }; }
  inline auto Abs( const v3& v ) -> v3    { return { Abs( v.x ), Abs( v.y ), Abs( v.z ) }; }
  inline auto Square( float v )           { return v * v; }
  inline auto Cube( float v )             { return v * v * v; }

  // -----------------
  // Misc?? / Unsorted / IEEE 754 Floating Point
  // -----------------

  bool IsNan( float );
  bool IsInf( float );

  auto SampleCosineWeightedHemisphere() -> SphericalCoordinate;
  auto SampleCosineWeightedHemisphere(v3 n) -> v3;

} // namespace Tac

