#include "tac_gjk.h" // self-inc

#include "tac-ecs/physics/tac_barycentric.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/containers/tac_optional.h"

namespace Tac
{

  

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
  static bool IsCorrectTetrahedronOrientation( const v3& a,
                                               const v3& b,
                                               const v3& c,
                                               const v3& d )
  {
    const v3 ab { b - a };
    const v3 ac { c - a };
    const v3 ad { d - a };
    const v3 abc { Cross( ab, ac ) };
    const bool result { Dot( abc, ad ) < 0 };
    return result;
  }

  SphereSupport::SphereSupport( v3 origin, float radius )
  {
    mOrigin = origin;
    mRadius = radius;
  }

  ConvexPolygonSupport::ConvexPolygonSupport( v3 obbPos,
                                              v3 obbHalfExtents,
                                              v3 obbEulerRads )
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
    const m4 transform { m4::Transform( obbHalfExtents, obbEulerRads, obbPos ) };
    for( v3& point : mPoints )
      point = ( transform * v4( point, 1.0f ) ).xyz();
  }

  v3 SphereSupport::GetFurthestPoint( const v3& dir ) const
  {
    TAC_ASSERT( Abs( dir.Length() - 1 ) < 0.001f );
    const v3 result { mOrigin + mRadius * dir };
    return result;
  }

  CapsuleSupport::CapsuleSupport( v3 base,
                                  float height,
                                  float radius )
  {
    const v3 up( 0, 1, 0 );
    mBotSpherePos = base + up * radius;
    mTopSpherePos = base + up * ( height - radius );
    mRadius = radius;
  }

  v3 CapsuleSupport::GetFurthestPoint( const v3& dir ) const
  {
    TAC_ASSERT( Abs( dir.Length() - 1 ) < 0.001f );
    const float botDot { Dot( dir, mBotSpherePos ) };
    const float topDot { Dot( dir, mTopSpherePos ) };
    return botDot > topDot
      ? mBotSpherePos + dir * mRadius
      : mTopSpherePos + dir * mRadius;
  }

  v3 ConvexPolygonSupport::GetFurthestPoint( const v3& dir ) const
  {
    int iLargestDot { 0 };
    float largestDot { -1 };
    const int pointCount { ( int )mPoints.size() };
    for( int i{}; i < pointCount; ++i )
    {
      const v3& point { mPoints[ i ] };
      const float dot { Dot( dir, point ) };
      if( dot < largestDot )
        continue;
      largestDot = dot;
      iLargestDot = i;
    }
    const v3 result = mPoints[ iLargestDot ];
    return result;
  }

  GJK::GJK( const Support* left,
            const Support* right )
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
      Swap( mSupports[ 2 ], mSupports[ 1 ] );
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
    CompoundSupport support { GetCompountSupport( mNormalizedSearchDir ) };
    const float dot { Dot( support.mDiffPt, mNormalizedSearchDir ) };
    if( dot < 0 )
    {
      mIsRunning = false;
      return;
    }

    mSupports.push_back( support );

    const v3 o  {};
    const int supportCount { ( int )mSupports.size() };
    switch( supportCount )
    {
      case 1:
      {
        const CompoundSupport& a { mSupports[ 0 ] };
        mClosestPoint = a.mDiffPt;
      } break;
      case 2:
      {
        const CompoundSupport& b { mSupports[ 0 ] };
        const CompoundSupport& a { mSupports[ 1 ] };
        const v3 ab { b.mDiffPt - a.mDiffPt };
        const v3 ao { o - a.mDiffPt };
        if( Dot( ao, ab ) > 0 )
        {
          const v3 bo { o - b.mDiffPt };
          const v3 ba { -ab };
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
        const CompoundSupport& c { mSupports[ 0 ] };
        const CompoundSupport& b { mSupports[ 1 ] };
        const CompoundSupport& a { mSupports[ 2 ] };

        const v3 ao { o - a.mDiffPt };
        const v3 ab { b.mDiffPt - a.mDiffPt };
        const v3 ac { c.mDiffPt - a.mDiffPt };

        const v3 bo { o - b.mDiffPt };
        const v3 ba { a.mDiffPt - b.mDiffPt };
        const v3 bc { c.mDiffPt - b.mDiffPt };

        const v3 co { o - c.mDiffPt };
        const v3 ca { a.mDiffPt - c.mDiffPt };
        const v3 cb { b.mDiffPt - c.mDiffPt };

        const v3 n { Cross( ac, ab ) };
        const v3 abOut { Cross( n, ab ) };
        const v3 acOut { Cross( ac, n ) };
        const v3 bcOut { Cross( n, bc ) };

        // vertex voronoi region
        if( Dot( ao, ab ) < 0 &&
            Dot( ao, ac ) < 0 )
        {
          mSupports = { a };
          mClosestPoint = a.mDiffPt;
        }
        else if( Dot( bo, ba ) < 0 &&
                 Dot( bo, bc ) < 0 )
        {
          mSupports = { b };
          mClosestPoint = b.mDiffPt;
        }
        else if( Dot( co, ca ) < 0 &&
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

        const auto& d { mSupports[ 0 ] };
        const auto& c { mSupports[ 1 ] };
        const auto& b { mSupports[ 2 ] };
        const auto& a { mSupports[ 3 ] };


        const v3 ao { o - a.mDiffPt };
        const v3 bo { o - b.mDiffPt };
        const v3 co { o - c.mDiffPt };
        const v3 d_o { o - d.mDiffPt };

        const v3 ab { b.mDiffPt - a.mDiffPt };
        const v3 ac { c.mDiffPt - a.mDiffPt };
        const v3 ad { d.mDiffPt - a.mDiffPt };

        const v3 ba { a.mDiffPt - b.mDiffPt };
        const v3 bc { c.mDiffPt - b.mDiffPt };
        const v3 bd { d.mDiffPt - b.mDiffPt };

        const v3 ca { a.mDiffPt - c.mDiffPt };
        const v3 cb { b.mDiffPt - c.mDiffPt };
        const v3 cd { d.mDiffPt - c.mDiffPt };

        const v3 da { a.mDiffPt - d.mDiffPt };
        const v3 db { b.mDiffPt - d.mDiffPt };
        const v3 dc { c.mDiffPt - d.mDiffPt };

        const v3 abc { Cross( ab, ac ) };
        const v3 bdc { Cross( bd, bc ) };
        const v3 acd { Cross( ac, ad ) };
        const v3 adb { Cross( ad, ab ) };

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
          mEPATriangles.clear();
          mEPATriangles.push_back( epaABC );
          mEPATriangles.push_back( epaBDC );
          mEPATriangles.push_back( epaADB );
          mEPATriangles.push_back( epaACD );
          return;
        }
      } break;
      default: TAC_ASSERT_INVALID_CASE( supportCount ); return;
    }

    const float searchDirLen { Length( mClosestPoint ) };
    if( searchDirLen < 0.001f )
    {
      mIsRunning = false;
      mIsColliding = true;

      // need more points to start a simplex for epa?
      const v3 supportDirs[] =
      {
        v3( 1, 0, 0 ),
        v3( 0, 1, 0 ),
        v3( 0, 0, 1 ),
        v3( -1, 0, 0 ),
        v3( 0, -1, 0 ),
        v3( 0, 0, -1 ),
      };

      for( const v3& supportDir : supportDirs )
      {
        const CompoundSupport compoundSupport { GetCompountSupport( supportDir ) };
        bool valid { true };
        for( const CompoundSupport& point : mSupports )
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
        const CompoundSupport& d = mSupports[ 0 ];
        const CompoundSupport& c = mSupports[ 1 ];
        const CompoundSupport& b = mSupports[ 2 ];
        const CompoundSupport& a = mSupports[ 3 ];
        const EPATriangle epaABC( a, b, c );
        const EPATriangle epaBDC( b, d, c );
        const EPATriangle epaADB( a, d, b );
        const EPATriangle epaACD( a, c, d );
        mEPATriangles.clear();
        mEPATriangles.push_back( epaABC );
        mEPATriangles.push_back( epaBDC );
        mEPATriangles.push_back( epaADB );
        mEPATriangles.push_back( epaACD );
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
    bool isFirstLoop { true };
    for( const EPATriangle& tri : mEPATriangles )
    {
      if( isFirstLoop || tri.mPlaneDist < mEPAClosest.mPlaneDist )
        mEPAClosest = tri;
      isFirstLoop = false;
    }

    mEPAClosestSupportPoint = GetCompountSupport( mEPAClosest.mNormal );
    // wrong --> mEPAClosestSupportPoint.mDiffPt.Lenght()
    const float pointDist { Dot( mEPAClosestSupportPoint.mDiffPt, mEPAClosest.mNormal ) };
    if( pointDist - mEPAClosest.mPlaneDist < 0.1f )
    {
      mEPAIsComplete = true;
      const v3 closestPointOnTri { mEPAClosest.mNormal * mEPAClosest.mPlaneDist };

      const BarycentricTriInput baryIn
      {
        .mP    { closestPointOnTri },
        .mTri0 { mEPAClosest.mV0.mDiffPt },
        .mTri1 { mEPAClosest.mV1.mDiffPt },
        .mTri2 { mEPAClosest.mV2.mDiffPt },
      };

      const Optional< BarycentricTriOutput > optBaryOut { BarycentricTriangle( baryIn ) };
      if( optBaryOut.HasValue() )
        return;

      mEPALeftPoint
        = mEPAClosest.mV0.mLeftPoint * optBaryOut->mBary0
        + mEPAClosest.mV1.mLeftPoint * optBaryOut->mBary1
        + mEPAClosest.mV2.mLeftPoint * optBaryOut->mBary2;
      mEPALeftNormal = mEPAClosest.mNormal;
      // this is degenerate?
      // v
        //= Normalize( Cross(
        //  mEPAClosest.mV1.mLeftPoint - mEPAClosest.mV0.mLeftPoint,
        //  mEPAClosest.mV2.mLeftPoint - mEPAClosest.mV0.mLeftPoint ) );
      mEPAPenetrationDist = mEPAClosest.mPlaneDist;
      return;
    }

    // why the fuck is this a list
    List< EPAHalfEdge > epaHalfEdges;
    auto iTri { mEPATriangles.begin() };
    while( iTri != mEPATriangles.end() )
    {
      const bool doesTriFacePoint{ Dot(
        iTri->mNormal,
        mEPAClosestSupportPoint.mDiffPt - iTri->GetArbitraryPointOnTriangle() ) > 0 };
      if( !doesTriFacePoint )
      {
        ++iTri;
        continue;
      }

      const EPAHalfEdge edges[]
      {
        EPAHalfEdge{ .mFrom = iTri->mV0, .mTo = iTri->mV1 },
        EPAHalfEdge{ .mFrom = iTri->mV1, .mTo = iTri->mV2 },
        EPAHalfEdge{ .mFrom = iTri->mV2, .mTo = iTri->mV0 },
      };

      for( const EPAHalfEdge& edge : edges )
      {
        const EPAHalfEdge reversed = edge.Reverse();
        if( auto iEdge{ epaHalfEdges.Find( reversed ) } )
          epaHalfEdges.erase( iEdge );
        else
          epaHalfEdges.push_back( edge );
      }

      iTri = mEPATriangles.erase( iTri );
      --mEPATriangleCount;
    }

    for( const EPAHalfEdge& edge : epaHalfEdges )
    {
      const EPATriangle tri( edge.mFrom, edge.mTo, mEPAClosestSupportPoint );
      mEPATriangles.push_back( tri );
      ++mEPATriangleCount;
    }
  }


  CompoundSupport GJK::GetCompountSupport( const v3& dir )
  {
    const v3 leftSupport { mLeft->GetFurthestPoint( dir ) };
    const v3 rightSupport { mRight->GetFurthestPoint( -dir ) };
    const v3 minkowskiDiffSupport { leftSupport - rightSupport };

    const CompoundSupport result
    {
      .mDiffPt { minkowskiDiffSupport },
      .mLeftPoint { leftSupport },
    };
    return result;
  }

  EPATriangle::EPATriangle( CompoundSupport v0,
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

  EPATriangle::EPATriangle( CompoundSupport v0,
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


  EPAHalfEdge EPAHalfEdge::Reverse() const
  {
    return EPAHalfEdge
    {
      .mFrom { mTo },
      .mTo   { mFrom },
    };
;
  }

  bool EPAHalfEdge::operator==( const EPAHalfEdge& other ) const
  {
    return mFrom.mDiffPt == other.mFrom.mDiffPt
      && mTo.mDiffPt == other.mTo.mDiffPt;
  }


} // namespace Tac

