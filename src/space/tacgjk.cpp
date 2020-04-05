#include "src/space/tacGjk.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/math/tacMatrix3.h"
#include "src/common/math/tacMatrix4.h"
#include <cmath>

namespace Tac
{
  void BarycentricTriangle(
    v3 p,
    v3 tri0,
    v3 tri1,
    v3 tri2,
    bool& fucked,
    float& bary0,
    float& bary1,
    float& bary2 )
  {
    v3 v0 = tri1 - tri0;
    v3 v1 = tri2 - tri0;
    v3 v2 = p - tri0;
    float d00 = Dot( v0, v0 );
    float d01 = Dot( v0, v1 );
    float d11 = Dot( v1, v1 );
    float d20 = Dot( v2, v0 );
    float d21 = Dot( v2, v1 );
    float denom = d00 * d11 - d01 * d01;
    fucked = std::abs( denom ) < 0.000001f;
    if( fucked )
      return;
    float invDenom = 1.0f / denom;
    float v = ( d11 * d20 - d01 * d21 ) * invDenom;
    float w = ( d00 * d21 - d01 * d20 ) * invDenom;
    float u = 1.0f - v - w;
    bary0 = u;
    bary1 = v;
    bary2 = w;
  }

  void BarycentricTetrahedron(
    v3 p,
    v3 tet0,
    v3 tet1,
    v3 tet2,
    v3 tet3,
    bool& fucked,
    float& bary0,
    float& bary1,
    float& bary2,
    float& bary3 )
  {
    v3 a = tet1 - tet0;
    v3 b = tet2 - tet0;
    v3 c = tet3 - tet0;
    v3 d = p - tet0;

    auto denominatorMatrix = m3::FromColumns( a, b, c );
    auto denominator = denominatorMatrix.determinant();
    fucked = std::abs( denominator ) < 0.001f;
    if( fucked )
      return;
    auto invDenominator = 1.0f / denominator;

    auto xNumeratorMatrix = m3::FromColumns( d, b, c );
    auto xNumerator = xNumeratorMatrix.determinant();
    auto x = xNumerator * invDenominator;

    auto yNumeratorMatrix = m3::FromColumns( a, d, c );
    auto yNumerator = yNumeratorMatrix.determinant();
    auto y = yNumerator * invDenominator;

    auto zNumeratorMatrix = m3::FromColumns( a, b, d );
    auto zNumerator = zNumeratorMatrix.determinant();
    auto z = zNumerator * invDenominator;

    bary0 = 1.0f - x - y - z;
    bary1 = x;
    bary2 = y;
    bary3 = z;
  }

  // Two possible orientations,
  //   
  //   orientation 1        orientation 2: 
  //   d is in front of     d is behind
  //   ( ab x ac )          ( ab x ac ) 
  //
  //            b                c
  //           /|\              /|\
  //          / | \            / | \
  //         /  |  d   --->   /  |  d
  //        /   | /          /   | /
  //       /    |/          /    |/
  //      a-----c          a-----b
  //
  // Ensure orientation 2
  bool IsCorrectTetrahedronOrientation( v3 a, v3 b, v3 c, v3 d )
  {
    v3 ab = b - a;
    v3 ac = c - a;
    v3 ad = d - a;
    auto abc = Cross( ab, ac );
    auto result = Dot( abc, ad ) < 0;
    return result;
  }

  SphereSupport::SphereSupport( v3 origin, float radius )
  {
    mOrigin = origin;
    mRadius = radius;
  }

  ConvexPolygonSupport::ConvexPolygonSupport( v3 obbPos, v3 obbHalfExtents, v3 obbEulerRads )
  {
    mPoints = {
      { -1, -1, -1 },
      { -1, -1, 1 },
      { -1, 1, -1 },
      { -1, 1, 1 },
      { 1, -1, -1 },
      { 1, -1, 1 },
      { 1, 1, -1 },
      { 1, 1, 1 },
    };
    auto transform = M4Transform( obbHalfExtents, obbEulerRads, obbPos );
    for( auto& point : mPoints )
      point = ( transform * v4( point, 1.0f ) ).xyz();
  }

  v3 SphereSupport::GetFurthestPoint( const v3& dir )
  {
    TAC_ASSERT( std::abs( dir.Length() - 1 ) < 0.001f );
    auto result = mOrigin + mRadius * dir;
    return result;
  }

  CapsuleSupport::CapsuleSupport( v3 base, float height, float radius )
  {
    v3 up( 0, 1, 0 );
    mBotSpherePos = base + up * radius;
    mTopSpherePos = base + up * ( height - radius );
    mRadius = radius;
  }

  v3 CapsuleSupport::GetFurthestPoint( const v3& dir )
  {
    TAC_ASSERT( std::abs( dir.Length() - 1 ) < 0.001f );
    auto botDot = Dot( dir, mBotSpherePos );
    auto topDot = Dot( dir, mTopSpherePos );

    v3 result;
    if( botDot > topDot )
    {
      result = mBotSpherePos + dir * mRadius;
    }
    else
    {
      result = mTopSpherePos + dir * mRadius;
    }
    return result;
  }

  v3 ConvexPolygonSupport::GetFurthestPoint( const v3& dir )
  {
    int iLargestDot = 0;
    float largestDot = -1;
    auto pointCount = ( int )mPoints.size();
    for( int i = 0; i < pointCount; ++i )
    {
      auto& point = mPoints[ i ];
      auto dot = Dot( dir, point );
      if( dot < largestDot )
        continue;
      largestDot = dot;
      iLargestDot = i;
    }
    auto result = mPoints[ iLargestDot ];
    return result;
  }

  GJK::GJK( Support* left, Support* right )
  {
    mLeft = left;
    mRight = right;
  }

  void GJK::EnsureCorrectTetrahedronOrientation()
  {
    if( !IsCorrectTetrahedronOrientation(
      mSupports[ 3 ].mDiffPt,
      mSupports[ 2 ].mDiffPt,
      mSupports[ 1 ].mDiffPt,
      mSupports[ 0 ].mDiffPt ) )
    {
      std::swap( mSupports[ 2 ], mSupports[ 1 ] );
    }
  }

  void GJK::Step()
  {
    TAC_ASSERT( mIsRunning );
    TAC_ASSERT( !mIsColliding );
    if( mIteration > 32 )
    {
      mIsRunning = false;
      return;
    }
    mIteration++;
    CompoundSupport support = GetCompountSupport( mNormalizedSearchDir );
    float dot = Dot( support.mDiffPt, mNormalizedSearchDir );
    if( dot < 0 )
    {
      mIsRunning = false;
      return;
    }

    mSupports.push_back( support );

    const v3 o = {};
    auto supportCount = ( int )mSupports.size();
    switch( supportCount )
    {
      case 1:
      {
        auto a = mSupports[ 0 ];
        mClosestPoint = a.mDiffPt;
      } break;
      case 2:
      {
        auto b = mSupports[ 0 ];
        auto a = mSupports[ 1 ];
        auto ab = b.mDiffPt - a.mDiffPt;
        auto ao = o - a.mDiffPt;
        if( Dot( ao, ab ) > 0 )
        {
          auto bo = o - b.mDiffPt;
          auto ba = -ab;
          if( Dot( bo, ba ) > 0 )
          {
            mClosestPoint = a.mDiffPt + Project( ab, ao );
          }
          else
          {
            mSupports = { b };
            mClosestPoint = b.mDiffPt;
          }
        }
        else
        {
          mSupports = { a };
          mClosestPoint = a.mDiffPt;
        }
      } break;
      case 3:
      {
        auto c = mSupports[ 0 ];
        auto b = mSupports[ 1 ];
        auto a = mSupports[ 2 ];

        auto ao = o - a.mDiffPt;
        auto ab = b.mDiffPt - a.mDiffPt;
        auto ac = c.mDiffPt - a.mDiffPt;

        auto bo = o - b.mDiffPt;
        auto ba = a.mDiffPt - b.mDiffPt;
        auto bc = c.mDiffPt - b.mDiffPt;

        auto co = o - c.mDiffPt;
        auto ca = a.mDiffPt - c.mDiffPt;
        auto cb = b.mDiffPt - c.mDiffPt;

        auto n = Cross( ac, ab );
        auto abOut = Cross( n, ab );
        auto acOut = Cross( ac, n );
        auto bcOut = Cross( n, bc );

        // vertex voronoi region
        if(
          Dot( ao, ab ) < 0 &&
          Dot( ao, ac ) < 0 )
        {
          mSupports = { a };
          mClosestPoint = a.mDiffPt;
        }
        else if(
          Dot( bo, ba ) < 0 &&
          Dot( bo, bc ) < 0 )
        {
          mSupports = { b };
          mClosestPoint = b.mDiffPt;
        }
        else if(
          Dot( co, ca ) < 0 &&
          Dot( co, cb ) < 0 )
        {
          mSupports = { c };
          mClosestPoint = c.mDiffPt;
        }
        // edge voronoi region
        else if( Dot( ao, abOut ) > 0 )
        {
          // edge ab
          mSupports = { a, b };
          mClosestPoint = a.mDiffPt + Project( ab, ao );
        }
        else if( Dot( ao, acOut ) > 0 )
        {
          // ac
          mSupports = { a, c };
          mClosestPoint = a.mDiffPt + Project( ac, ao );
        }
        else if( Dot( bo, bcOut ) > 0 )
        {
          // bc
          mSupports = { b, c };
          mClosestPoint = b.mDiffPt + Project( bc, bo );
        }
        // triangle voronoi region
        else
        {
          mClosestPoint = Project( n, a.mDiffPt );
        }
      } break;
      case 4:
      {
        EnsureCorrectTetrahedronOrientation();

        auto d = mSupports[ 0 ];
        auto c = mSupports[ 1 ];
        auto b = mSupports[ 2 ];
        auto a = mSupports[ 3 ];


        auto ao = o - a.mDiffPt;
        auto bo = o - b.mDiffPt;
        auto co = o - c.mDiffPt;
        auto d_o = o - d.mDiffPt;

        auto ab = b.mDiffPt - a.mDiffPt;
        auto ac = c.mDiffPt - a.mDiffPt;
        auto ad = d.mDiffPt - a.mDiffPt;

        auto ba = a.mDiffPt - b.mDiffPt;
        auto bc = c.mDiffPt - b.mDiffPt;
        auto bd = d.mDiffPt - b.mDiffPt;

        auto ca = a.mDiffPt - c.mDiffPt;
        auto cb = b.mDiffPt - c.mDiffPt;
        auto cd = d.mDiffPt - c.mDiffPt;

        auto da = a.mDiffPt - d.mDiffPt;
        auto db = b.mDiffPt - d.mDiffPt;
        auto dc = c.mDiffPt - d.mDiffPt;

        auto abc = Cross( ab, ac );
        auto bdc = Cross( bd, bc );
        auto acd = Cross( ac, ad );
        auto adb = Cross( ad, ab );

        if( // vertex a
          Dot( ao, ab ) < 0 &&
          Dot( ao, ac ) < 0 &&
          Dot( ao, ad ) < 0 )
        {
          mSupports = { a };
          mClosestPoint = a.mDiffPt;
        }
        else if( // vertex b
          Dot( bo, ba ) < 0 &&
          Dot( bo, bc ) < 0 &&
          Dot( bo, bd ) < 0 )
        {
          mSupports = { b };
          mClosestPoint = b.mDiffPt;
        }
        else if( // vertex c
          Dot( co, ca ) < 0 &&
          Dot( co, cb ) < 0 &&
          Dot( co, cd ) < 0 )
        {
          mSupports = { c };
          mClosestPoint = c.mDiffPt;
        }
        else if( // vertex d
          Dot( d_o, da ) < 0 &&
          Dot( d_o, db ) < 0 &&
          Dot( d_o, dc ) < 0 )
        {
          mSupports = { d };
          mClosestPoint = d.mDiffPt;
        }
        else if( // edge ab
          Dot( ao, Cross( ab, abc ) ) > 0 &&
          Dot( ao, Cross( adb, ab ) ) > 0 )
        {
          mSupports = { a, b };
          mClosestPoint = a.mDiffPt + Project( ab, ao );
        }
        else if( // edge ac
          Dot( ao, Cross( abc, ac ) ) > 0 &&
          Dot( ao, Cross( ac, acd ) ) > 0 )
        {
          mSupports = { a, c };
          mClosestPoint = a.mDiffPt + Project( ac, ao );
        }
        else if( // edge ad
          Dot( ao, Cross( ad, adb ) ) > 0 &&
          Dot( ao, Cross( acd, ad ) ) > 0 )
        {
          mSupports = { a, d };
          mClosestPoint = a.mDiffPt + Project( ad, ao );
        }
        else if( // edge bc
          Dot( bo, Cross( bc, abc ) ) > 0 &&
          Dot( bo, Cross( bdc, bc ) ) > 0 )
        {
          mSupports = { b, c };
          mClosestPoint = b.mDiffPt + Project( bc, bo );
        }
        else if( // edge bd
          Dot( bo, Cross( bd, bdc ) ) > 0 &&
          Dot( bo, Cross( adb, bd ) ) > 0 )
        {
          mSupports = { b, d };
          mClosestPoint = b.mDiffPt + Project( bd, bo );
        }
        else if( // edge cd
          Dot( co, Cross( cd, acd ) ) > 0 &&
          Dot( co, Cross( bdc, cd ) ) > 0 )
        {
          mSupports = { c, d };
          mClosestPoint = c.mDiffPt + Project( cd, co );
        }
        else if( // face abc
          Dot( ao, abc ) > 0 )
        {
          mSupports = { a, b, c };
          mClosestPoint = Project( abc, a.mDiffPt );
        }
        else if( // face abd
          Dot( ao, adb ) > 0 )
        {
          mSupports = { a, b, d };
          mClosestPoint = Project( Normalize( adb ), a.mDiffPt );
        }
        else if( // face acd
          Dot( ao, acd ) > 0 )
        {
          mSupports = { a, c, d };
          mClosestPoint = Project( Normalize( acd ), a.mDiffPt );
        }
        else if( // face bcd
          Dot( bo, bdc ) > 0 )
        {
          mSupports = { b, c, d };
          mClosestPoint = Project( bdc, b.mDiffPt );
        }
        else // inside the simplex
        {
          mIsRunning = false;
          mIsColliding = true;

          EPATriangle epaABC( a, b, c, abc );
          EPATriangle epaBDC( b, d, c, bdc );
          EPATriangle epaADB( a, d, b, adb );
          EPATriangle epaACD( a, c, d, acd );
          mEPATriangles = {
            epaABC,
            epaBDC,
            epaADB,
            epaACD };
          return;
        }
      } break;
      TAC_INVALID_DEFAULT_CASE( supportCount );
    }

    auto searchDirLen = Length( mClosestPoint );
    if( searchDirLen < 0.001f )
    {
      mIsRunning = false;
      mIsColliding = true;

      // need more points to start a simplex for epa?
      for( auto supportDir : {
        v3( 1, 0, 0 ),
        v3( 0, 1, 0 ),
        v3( 0, 0, 1 ),
        v3( -1, 0, 0 ),
        v3( 0, -1, 0 ),
        v3( 0, 0, -1 ) } )
      {
        CompoundSupport compoundSupport = GetCompountSupport( supportDir );
        bool valid = true;
        for( auto point : mSupports )
        {
          if( Quadrance( point.mDiffPt, compoundSupport.mDiffPt ) < 0.001f )
          {
            valid = false;
            break;
          }
        }
        if( valid )
          mSupports.push_back( compoundSupport );
        if( mSupports.size() == 4 )
          break;
      }
      if( mSupports.size() == 4 )
      {
        EnsureCorrectTetrahedronOrientation();
        auto d = mSupports[ 0 ];
        auto c = mSupports[ 1 ];
        auto b = mSupports[ 2 ];
        auto a = mSupports[ 3 ];
        EPATriangle epaABC( a, b, c );
        EPATriangle epaBDC( b, d, c );
        EPATriangle epaADB( a, d, b );
        EPATriangle epaACD( a, c, d );
        mEPATriangles = {
          epaABC,
          epaBDC,
          epaADB,
          epaACD };
      }
      return;
    }
    mNormalizedSearchDir = mClosestPoint / -searchDirLen;
  }

  void GJK::EPAStep()
  {
    TAC_ASSERT( !mIsRunning );
    TAC_ASSERT( mIsColliding );
    TAC_ASSERT( !mEPAIsComplete );
    mEPAIteration++;

    // We are trying to find the face on hull that is closest to the origin
    bool isFirstLoop = true;
    for( auto tri : mEPATriangles )
    {
      if( isFirstLoop || tri.mPlaneDist < mEPAClosest.mPlaneDist )
        mEPAClosest = tri;
      isFirstLoop = false;
    }

    mEPAClosestSupportPoint = GetCompountSupport( mEPAClosest.mNormal );
    // wrong --> mEPAClosestSupportPoint.mDiffPt.Lenght()
    auto pointDist = Dot( mEPAClosestSupportPoint.mDiffPt, mEPAClosest.mNormal );
    if( pointDist - mEPAClosest.mPlaneDist < 0.1f )
    {
      mEPAIsComplete = true;
      float bary0;
      float bary1;
      float bary2;
      v3 closestPointOnTri = mEPAClosest.mNormal * mEPAClosest.mPlaneDist;
      BarycentricTriangle(
        closestPointOnTri,
        mEPAClosest.mV0.mDiffPt,
        mEPAClosest.mV1.mDiffPt,
        mEPAClosest.mV2.mDiffPt,
        mEPABarycentricFucked,
        bary0,
        bary1,
        bary2 );
      if( mEPABarycentricFucked )
        return;
      mEPALeftPoint
        = mEPAClosest.mV0.mLeftPoint*bary0
        + mEPAClosest.mV1.mLeftPoint*bary1
        + mEPAClosest.mV2.mLeftPoint*bary2;
      mEPALeftNormal = mEPAClosest.mNormal;
      // this is degenerate?
      // v
        //= Normalize( Cross(
        //  mEPAClosest.mV1.mLeftPoint - mEPAClosest.mV0.mLeftPoint,
        //  mEPAClosest.mV2.mLeftPoint - mEPAClosest.mV0.mLeftPoint ) );
      mEPAPenetrationDist = mEPAClosest.mPlaneDist;
      return;
    }

    std::list< EPAHalfEdge > epaHalfEdges;
    auto iTri = mEPATriangles.begin();
    while( iTri != mEPATriangles.end() )
    {
      bool doesTriFacePoint = Dot(
        iTri->mNormal,
        mEPAClosestSupportPoint.mDiffPt - iTri->GetArbitraryPointOnTriangle() ) > 0;
      if( !doesTriFacePoint )
      {
        ++iTri;
        continue;
      }
      for( auto edge : {
        EPAHalfEdge( iTri->mV0, iTri->mV1 ),
        EPAHalfEdge( iTri->mV1, iTri->mV2 ),
        EPAHalfEdge( iTri->mV2, iTri->mV0 ) } )
      {
        EPAHalfEdge reversed = edge.Reverse();
        auto iEdge = std::find( epaHalfEdges.begin(), epaHalfEdges.end(), reversed );
        if( iEdge == epaHalfEdges.end() )
        {
          epaHalfEdges.push_back( edge );
        }
        else
        {
          epaHalfEdges.erase( iEdge );
        }
      }
      iTri = mEPATriangles.erase( iTri );
      --mEPATriangleCount;
    }
    for( auto edge : epaHalfEdges )
    {
      EPATriangle tri(
        edge.mFrom,
        edge.mTo,
        mEPAClosestSupportPoint );
      mEPATriangles.push_back( tri );
      ++mEPATriangleCount;
    }
  }


  CompoundSupport GJK::GetCompountSupport( const v3& dir )
  {
    auto leftSupport = mLeft->GetFurthestPoint( dir );
    auto rightSupport = mRight->GetFurthestPoint( -dir );
    auto minkowskiDiffSupport = leftSupport - rightSupport;
    CompoundSupport result;
    result.mLeftPoint = leftSupport;
    result.mDiffPt = minkowskiDiffSupport;
    return result;
  }

  EPATriangle::EPATriangle(
    CompoundSupport v0,
    CompoundSupport v1,
    CompoundSupport v2,
    v3 toNormalize )
  {
    mV0 = v0;
    mV1 = v1;
    mV2 = v2;
    mNormal = Normalize( toNormalize );
    ComputeDist();
  }

  EPATriangle::EPATriangle(
    CompoundSupport v0,
    CompoundSupport v1,
    CompoundSupport v2 )
  {
    mV0 = v0;
    mV1 = v1;
    mV2 = v2;
    auto e1 = mV1.mDiffPt - mV0.mDiffPt;
    auto e2 = mV2.mDiffPt - mV0.mDiffPt;
    mNormal = Normalize( Cross( e1, e2 ) );
    ComputeDist();
  }

  v3 EPATriangle::GetArbitraryPointOnTriangle()
  {
    // could be any of mv0, mv1, mv2
    return mV0.mDiffPt;
  }

  void EPATriangle::ComputeDist()
  {
    mPlaneDist = Dot( GetArbitraryPointOnTriangle(), mNormal );
    //Assert( mPlaneDist >= 0.0f );
  }

  EPAHalfEdge::EPAHalfEdge( CompoundSupport from, CompoundSupport to )
  {
    mFrom = from;
    mTo = to;
  }

  EPAHalfEdge EPAHalfEdge::Reverse()
  {
    EPAHalfEdge result;
    result.mFrom = mTo;
    result.mTo = mFrom;
    return result;
  }

  bool EPAHalfEdge::operator==( const EPAHalfEdge& other ) const
  {
    return mFrom.mDiffPt == other.mFrom.mDiffPt
      && mTo.mDiffPt == other.mTo.mDiffPt;
  }


}

