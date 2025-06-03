#include "tac_math.h" // self-inc

#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector3i.h"

#if TAC_SHOULD_IMPORT_STD()
  import std; // cstdlib, algorithm, cmath
#else
  #include <cmath>
  #include <random>
#endif

// -------------
// Interpolation
// -------------

namespace Tac
{
  static std::mt19937 sRNG;
}

auto Tac::Min( const v2& a, const v2& b ) -> Tac::v2    { return { Min( a.x, b.x ), Min( a.y, b.y ) }; }
auto Tac::Max( const v2& a, const v2& b ) -> Tac::v2    { return { Max( a.x, b.x ), Max( a.y, b.y ) }; }
auto Tac::Min( const v2i& a, const v2i& b ) -> Tac::v2i { return { Min( a.x, b.x ), Min( a.y, b.y ) }; }
auto Tac::Max( const v2i& a, const v2i& b ) -> Tac::v2i { return { Max( a.x, b.x ), Max( a.y, b.y ) }; }
auto Tac::Min( const v3& a, const v3& b ) -> Tac::v3    { return { Min( a.x, b.x ), Min( a.y, b.y ), Min( a.z, b.z ) }; }
auto Tac::Max( const v3& a, const v3& b ) -> Tac::v3    { return { Max( a.x, b.x ), Max( a.y, b.y ), Max( a.z, b.z ) }; }
auto Tac::Min( const v3i& a, const v3i& b ) -> Tac::v3i { return { Min( a.x, b.x ), Min( a.y, b.y ), Min( a.z, b.z ) }; }
auto Tac::Max( const v3i& a, const v3i& b ) -> Tac::v3i { return { Max( a.x, b.x ), Max( a.y, b.y ), Max( a.z, b.z ) }; }

auto Tac::EaseInOutQuart( float t ) -> float { return t < 0.5f ? ( 8 * t * t * t * t ) : ( 1 - 8 * ( t - 1 ) * ( t - 1 ) * ( t - 1 ) * ( t - 1 ) ); }

void Tac::Spring( float* posCurrent,
                  float* velCurrent,
                  float posTarget,
                  float springyness,
                  float deltaTimeSeconds )
{
  // http://allenchou.net/2015/04/game-math-precise-control-over-numeric-springing/
  const float x { *posCurrent };
  const float v { *velCurrent };
  const float f { 1.0f + 2.0f * deltaTimeSeconds * springyness };
  const float oo { springyness * springyness };
  const float hoo { deltaTimeSeconds * oo };
  const float hhoo { deltaTimeSeconds * hoo };
  const float detInv { 1.0f / ( f + hhoo ) };
  const float detX { f * x + deltaTimeSeconds * v + hhoo * posTarget };
  const float detV { v + hoo * ( posTarget - x ) };
  *posCurrent = detX * detInv;
  *velCurrent = detV * detInv;
}



auto Tac::Saturate( const float value ) -> float
{
  return Clamp( value, 0.0f, 1.0f );
}

auto Tac::Clamp( float x, float xMin, float xMax ) -> float
{
  if( x < xMin )
    return xMin;

  if( x > xMax )
    return xMax;

  return x;
}

// ------------
// Trigonometry
// ------------

auto Tac::Sin( float f ) -> float               { return std::sinf( f ); }
auto Tac::Sin( double d ) -> double              { return std::sin( d );  }
auto Tac::Cos( float f ) -> float               { return std::cosf( f ); }
auto Tac::Cos( double d ) -> double              { return std::cos( d );  }
auto Tac::Tan( float f ) -> float               { return std::tanf( f ); }
auto Tac::Tan( double d ) -> double              { return std::tan( d );  }
auto Tac::Atan2( float y, float x ) -> float    { return std::atan2( y, x ); }
auto Tac::Asin( float f ) -> float              { return std::asinf( f ); }
auto Tac::Acos( float f ) -> float              { return std::acosf( f ); }

// --------
// Rounding
// --------

auto Tac::Round( float f ) -> float              { return std::roundf( f ); }

// Example:
//   RoundUpToNearestMultiple( 10, 5 ) = 10
//   RoundUpToNearestMultiple( 10, 4 ) = 12
//constexpr int Tac::RoundUpToNearestMultiple( int numToRound, int multiple )
//{
//  return ( ( numToRound + ( multiple - 1 ) ) / multiple ) * multiple;
//}

auto Tac::Floor( float f ) -> float           { return std::floor( f ); }
auto Tac::Floor( double d ) -> double          { return std::floor( d ); }
auto Tac::Floor( const v2& v ) -> Tac::v2       { return v2( Floor( v.x ), Floor( v.y ) ); }
auto Tac::Floor( const v3& v ) -> Tac::v3       { return v3( Floor( v.x ), Floor( v.y ), Floor( v.z ) ); }
auto Tac::Ceil( float f ) -> float            { return std::ceil( f ); }
auto Tac::Ceil( const v2& v ) -> Tac::v2        { return v2( Ceil( v.x ), Ceil( v.y ) ); }
auto Tac::Ceil( const v3& v ) -> Tac::v3        { return v3( Ceil( v.x ), Ceil( v.y ), Ceil( v.z ) ); }
auto Tac::Fmod( float x, float y ) -> float   { return std::fmodf( x, y ); }
auto Tac::Fmod( double x, double y ) -> double { return std::fmod( x, y ); }

// ----------
// Randomness
// ----------


auto Tac::RandomFloat0To1() -> float                      { return ( float )sRNG() / sRNG.max(); }
auto Tac::RandomIndex( int n ) -> int                     { return sRNG() % n; }
auto Tac::RandomFloatMinus1To1() -> float                 { return ( RandomFloat0To1() * 2.0f ) - 1.0f; }
auto Tac::RandomFloatBetween( float a, float b ) -> float { return Lerp( a, b, RandomFloat0To1() ); }

auto Tac::RandomPointInTriangle( v3 p0, v3 p1, v3 p2 ) -> Tac::v3
{
  float u{ RandomFloat0To1() };
  float v{ RandomFloat0To1() };
  if( u + v > 1 ) { u = 1 - u; v = 1 - v; }
  return p0 + u * ( p1 - p0 ) + v * ( p2 - p0 );
}


namespace Tac
{
  auto SphericalCoordinate::ToCartesian() const -> v3
  {
    return v3( mRadius * Cos( mPhi ) * Sin( mTheta ),
               mRadius * Cos( mTheta ),
               mRadius * Sin( mPhi ) * Sin( mTheta ) );
  }

  auto SphericalCoordinate::FromCartesian(v3 v) -> SphericalCoordinate
  {
    const float x = v.x;
    const float y = v.y;
    const float z = v.z;
    const float q { x * x + y * y + z * z };
    if( q < 0.01f )
      return {};
    const float r { Sqrt( q ) };
    const float t { Acos( y / r ) };
    const float p { Atan2( z, x ) };
    return SphericalCoordinate
    {
      .mRadius { r },
      .mTheta  { t },
      .mPhi    { p },
    };
  }
}

// ------------
// Intersection
// ------------

auto Tac::RaySphere( const v3& rayPos, const v3& rayDir, const v3& spherePos, float sphereRadius ) -> float
{
  const v3 dp { rayPos - spherePos };
  const float c { Dot( dp, dp ) - Square( sphereRadius ) };
  const float b { 2 * Dot( dp, rayDir ) };
  const float a { Dot( rayDir, rayDir ) };
  const float discriminant { Square( b ) - 4 * a * c };
  if( discriminant < 0 )
    return -1;
  const float rt { Sqrt( discriminant ) };
  const float signs[]{ -1, 1 };
  for( const float sign : signs )
  {
    const float t { ( -b + sign * rt ) / ( 2 * a ) };
    if( t > 0 )
      return t;
  }
  return -1;
}

auto Tac::ClosestPointLineSegment( const v3& p0, const v3& p1, const v3& p ) -> Tac::v3
{
  const v3 to1 { p1 - p0 };
  const v3 toP { p - p0 };
  const float toP_dot_to1 { Dot( toP, to1 ) };
  if( toP_dot_to1 < 0 )
    return p0;

  const float q { Dot( to1, to1 ) };
  if( q < 0.001f )
    return p0;

  const float t { toP_dot_to1 / q };
  if( t > 1 - 0.001f )
    return p1;

  return p0 + to1 * t;
}

auto Tac::Fract( float f ) -> float
{
  float integralpart;
  float fractionalpart { std::modf( f, &integralpart ) };
  return fractionalpart;
}

auto Tac::Abs( const v2& v ) -> Tac::v2         { return { Abs( v.x ), Abs( v.y ) }; }
auto Tac::Abs( const v3& v ) -> Tac::v3         { return { Abs( v.x ), Abs( v.y ), Abs( v.z ) }; }
auto Tac::Pow( float base, float exp ) -> float { return std::pow( base, exp ); }
auto Tac::Sqrt( float f ) -> float              { return std::sqrtf( f ); }
auto Tac::Exp( float f ) -> float               { return std::exp( f ); }

// -----------------
// Misc?? / Unsorted
// -----------------

auto Tac::IsNan( float f ) -> bool              { return std::isnan( f ); }
auto Tac::IsInf( float f ) -> bool              { return std::isinf( f ); }

namespace Tac
{
  auto ClosestPointLineSegments::Solve( Input input) -> ClosestPointLineSegments::Output
  {
    const v3 p1{input.mLine1Begin};
    const v3 q1{input.mLine1End};
    const v3 p2{input.mLine2Begin};
    const v3 q2{input.mLine2End};
    // this function ripped from real time collision detection
    float s { -1 };
    float t { -1 };
    const float EPSILON { 0.001f };
    v3 d1 { q1 - p1 };
    v3 d2 { q2 - p2 };
    v3 r { p1 - p2 };
    float a { Dot( d1, d1 ) };
    float e { Dot( d2, d2 ) };
    float f { Dot( d2, r ) };
    if( a <= EPSILON && e <= EPSILON )
    {
      s = 0;
      t = 0;
    }
    else if( a <= EPSILON )
    {
      s = 0;
      t = f / e;
      t = Saturate( t);
    }
    else if( e <= EPSILON )
    {
      float c { Dot( d1, r ) };
      t = 0;
      s = Saturate( -c / a );
    }
    else
    {
      float c { Dot( d1, r ) };
      float b { Dot( d1, d2 ) };
      float denom { a * e - b * b };
      s = denom ? Saturate( ( b * f - c * e ) / denom ) : 0;
      t = ( b * s + f ) / e;
      if( t < 0 )
      {
        t = 0;
        s = Saturate( -c / a);
      }
      else if( t > 1 )
      {
        t = 1;
        s = Saturate( ( b - c ) / a);
      }
    }
    const v3 c1 { p1 + d1 * s };
    const v3 c2 { p2 + d2 * t };
    return Output
    {
      .mClosestPointOnLine1{ c1 },
      .mClosestPointOnLine2{ c2 },
    };
  }

  auto ClosestPointTwoRays::Solve( Input input ) -> ClosestPointTwoRays::Output
  {
    // http://palitri.com/vault/stuff/maths/Rays%20closest%20point.pdf
    //
    //              \ /
    //               \
    //              / \
    //             /   \
    //            /     \    
    //           /   z__\.D 
    //         E.__---  / \
    //         /           \
    //        /             \
    //       /               \
    //     _/                 \_
    // 2nd /| b               |\ a
    // ray/                     \ first ray
    //  B. <-------- c ----------. A
    //
    //     A - first ray origin
    //     a - first ray direction ( not necessarily normalized )
    //     B - second ray origin
    //     b - second ray direction ( not necessarily normalized )
    //     c - vector from A to B ( c = B - A )
    //     D - closest point along the first ray to the second ray
    //     E - closest point along the second ray to the first ray
    //
    // +-------------+
    // | 3 Equations |
    // +-------------+
    // |
    // +--> Dot( a, z ) = 0
    // +--> Dot( b, z ) = 0
    // +--> c + (b*e) + z - (a*d) = 0
    //
    // +------------+
    // | 3 unknowns |
    // +------------+
    // |
    // +--> e - scalar such that b*e=E
    // +--> d - scalar such that a*d=D
    // +--> z - vector perpendicular to both a and b ( z = D - E )

    const v3 A{ input.mRay0Pos };
    const v3 a{ input.mRay0Dir };
    const v3 B{ input.mRay1Pos };
    const v3 b{ input.mRay1Dir };
    const v3 c{ B - A };
    const float ab{ Dot( a, b ) };
    const float bc{ Dot( b, c ) };
    const float ac{ Dot( a, c ) };
    const float aa{ Dot( a, a ) };
    const float bb{ Dot( b, b ) };
    const float denom{ aa * bb - ab * ab };
    const float d{ ( -ab * bc + ac * bb ) / denom };
    const float e{ ( ab * ac - bc * aa ) / denom };
    return Output
    {
      .mRay0T { d },
      .mRay1T { e },
    };
  }

  auto RayTriangle::Output::GetIntersectionPoint( const Triangle& tri ) const -> v3
  {
    const v3& A{ tri.mP0 };
    const v3& B{ tri.mP1 };
    const v3& C{ tri.mP2 };
    const float u { mU };
    const float v { mV };
    const float w { 1.0f - u - v };
    return A * w + B * u + C * v;
  }

  auto RayTriangle::Solve( const Ray& ray, const Triangle& triangle ) -> RayTriangle::Output
  {
    // Moller-Trumbore algorithm - "Fast, Minimum Storage Ray/Triangle Intersection" (1977)
    const v3& p0{ triangle.mP0 };
    const v3& p1{ triangle.mP1 };
    const v3& p2{ triangle.mP2 };
    const v3& rayPos{ ray.mOrigin };
    const v3& rayDir{ ray.mDirection };

    const v3 edge2{ p2 - p0 };
    const v3 edge1{ p1 - p0 };
    const v3 b{ rayPos - p0 };
    const v3 p{ Cross( rayDir, edge2 ) };
    const v3 q{ Cross( b, edge1 ) };
    const float pdotv1{ Dot( p, edge1 ) };
    const float t{ Dot( q, edge2 ) / pdotv1 };
    const float u{ Dot( p, b ) / pdotv1 };
    const float v{ Dot( q, rayDir ) / pdotv1 };
    if( t < 0 || u < 0 || v < 0 || u + v > 1 )
      return {};

    return RayTriangle::Output
    {
      .mT{ t },
      .mU{ u },
      .mV{ v },
      .mValid{ true },
    };
  }
} // namespace Tac

auto Tac::SampleCosineWeightedHemisphere() -> Tac::SphericalCoordinate
{
  // Starting off, a pdf must be normalized
  //   \int_{H^2} p(\omega) d\omega = 1
  //
  // We would like p(omega) \propto \cos(\theta), or in other words, p(\omega) = c * \cos(\theta)
  //   \int_{H^2} c * cos(\theta) d\omega = 1
  // 
  // solving for c...
  //   \int_0^{2\pi} \int_0^{\frac{\pi}{2}} c * \cos(\theta) \sin(\theta) d\theta d\phi = 1
  //   ...
  //   c = 1/pi
  //
  // plugging c into p(omega) we get
  //              cos(theta)
  //   p(omega) = ----------
  //                  pi
  //
  //
  // from (magic??? change of variables from spherical to cartesian??? ),
  // we know that p(theta,phi) = sin(theta)p(omega)
  //
  //                   sin(theta) cos(theta)
  //   p(theta, phi) = ---------------------
  //                            pi
  //
  // Next get the marginal density function for theta by "integrating out" phi
  //   p(theta) = \int_0^{2\pi} p(theta, phi) dphi
  //            = ...
  //            = 2 sin(theta) cos(theta)
  //
  // Next get the conditional density function for phi
  //                  pdf(theta, phi)          1
  //   p(phi|theta) = --------------- = ... = ---
  //                    pdf(theta)            2pi
  //
  // p(theta,phi) = p(theta) p(phi|theta)
  //
  // The d/dx CDF = pdf, so integrate each pdf to get the CDFs
  //   P(x) = \int_0^\x p(x^\prime) dx^\prime
  //
  //   P(theta) = \int_0^\theta p(theta^\prime) d\theta^\prime
  //            = \int_0^\theta 2 sin(theta) cos( theta) d\theta^\prime
  //            = ...
  //            = \sin^2(\theta)
  //
  //   P(phi|theta) = \int_0^\phi p(phi^\prime) d\phi^\prime
  //                = \int_0^\phi \frac{1}{2\pi} d\phi^\prime
  //                = ...
  //                = \frac{\phi}{2\pi}
  //
  // The CDF is sampled with uniform random number \xi \in [0,1] using the inversion method
  //
  //           1                                  1              +----+
  //           |                                  |              | p4 |
  //           |                                  |         +----+----+
  //           |                                  |         | p3 | p3 |
  //     pdf(X)|                            CDF(X)|         |    |    |
  //           |                                  |         |    |    |
  //           |                                  |    +----+----+----+
  //           |         +----+                   |    | p2 | p2 | p2 |
  //           |    +----+ p3 |                   |    |    |    |    |
  //           +----+ p2 |    +----+              +----+----+----+----+
  //           | p1 |    |    | p4 |              | p1 | p1 | p1 | p1 |
  //           0----+----+----+----+              0----+----+----+----+
  //                     X                                  X
  //
  // If we assign canonical uniform random variable \xi has value CDF(X_i) with probability p(X_i)
  // If we let CDF(X_i) = \xi, then by inverting the CDF we can solve for X_i given \xi
  //
  //   P(theta) = \xi                   P(P^{-1}(\theta)) = \theta
  //   theta = P^{-1}(\xi)          sin^2(P^{-1}(\theta)) = \theta
  //           |                      sin(P^{-1}(\theta)) = \sqrt(\theta)
  //           |                          P^{-1}(\theta)) = \sin^-1(\sqrt(\theta))
  //           |                          |
  //           +--------------------------+
  //           |
  //           v
  //   theta = sin^-1(\sqrt(\xi))
  //
  // Do the same for phi
  //
  //   ...
  //   \phi = 2 pi \xi_{\phi}
  //

  float xi_theta = RandomFloat0To1();
  float xi_phi = RandomFloat0To1();
  float theta = Asin( Sqrt( xi_theta ) );
  float phi = 2 * 3.14f * xi_phi;

  return SphericalCoordinate
  {
    .mTheta { theta },
    .mPhi   { phi },
  };
  
  // https://ameye.dev/notes/sampling-the-hemisphere/
  // https://agraphicsguynotes.com/posts/sample_microfacet_brdf/
  // https://pbr-book.org/4ed/Monte_Carlo_Integration/Sampling_Using_the_Inversion_Method
  // https://pbr-book.org/4ed/Sampling_Algorithms/Sampling_Multidimensional_Functions
}

auto Tac::SampleCosineWeightedHemisphere(v3 n) -> v3
{
  auto spherical = SampleCosineWeightedHemisphere();
  v3 tan0;
  v3 tan1;
  GetFrameRH(n, tan0, tan1);
  v3 cartesian = spherical.ToCartesian();
  return
    cartesian.x * tan0 +
    cartesian.y * n +
    cartesian.z * tan1;
}

