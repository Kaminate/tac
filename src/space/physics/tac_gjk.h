// The Gilbert–Johnson–Keerthi algorithm computes the intersection
// of two convex polygons. We use it for physics integration and collision detection

#pragma once

#include "src/common/containers/tac_vector.h"
#include "src/common/containers/tac_list.h"
#include "src/common/math/tac_vector3.h"

namespace Tac
{
  struct Support
  {
    virtual v3 GetFurthestPoint( const v3& dir ) const = 0;
  };

  struct SphereSupport : public Support
  {
    SphereSupport() = default;
    SphereSupport( v3 origin,
                   float radius );
    v3 GetFurthestPoint( const v3& dir ) const override;
    v3 mOrigin = {};
    float mRadius = 0;
  };

  struct CapsuleSupport : public Support
  {
    CapsuleSupport() = default;
    CapsuleSupport( v3 base,
                    float height,
                    float radius );
    v3 GetFurthestPoint( const v3& dir ) const override;
    v3 mBotSpherePos = {};
    v3 mTopSpherePos = {};
    float mRadius = 0;
  };

  struct ConvexPolygonSupport : public Support
  {
    ConvexPolygonSupport() = default;
    ConvexPolygonSupport( v3 obbPos,
                          v3 obbHalfExtents,
                          v3 obbEulerRads );
    v3 GetFurthestPoint( const v3& dir ) const override;
    Vector< v3 > mPoints;
  };

  struct CompoundSupport
  {
    // what is a diffpt
    v3 mDiffPt = {};

    // what is a leftpt
    v3 mLeftPoint = {};
  };

  struct EPATriangle
  {
    EPATriangle() = default;
    EPATriangle( CompoundSupport v0,
                 CompoundSupport v1,
                 CompoundSupport v2,
                 v3 toNormalize );
    EPATriangle( CompoundSupport v0,
                 CompoundSupport v1,
                 CompoundSupport v2 );
    v3 GetArbitraryPointOnTriangle();
    void ComputeDist();

    CompoundSupport mV0;
    CompoundSupport mV1;
    CompoundSupport mV2;
    v3 mNormal = {};
    float mPlaneDist = 0;
  };

  struct EPAHalfEdge
  {
    EPAHalfEdge Reverse() const;
    bool operator == ( const EPAHalfEdge& other ) const;

    CompoundSupport mFrom;
    CompoundSupport mTo;
  };

  struct GJK
  {
    GJK() = default;
    GJK( const Support* left,
         const Support* right );
    void EnsureCorrectTetrahedronOrientation();
    void Step();
    void EPAStep();
    CompoundSupport GetCompountSupport( const v3& dir );

    bool mIsRunning = true;
    bool mIsColliding = false;
    int mIteration = 0;
    v3 mNormalizedSearchDir = { 1, 0, 0 };
    v3 mClosestPoint = {};
    Vector< CompoundSupport > mSupports;
    const Support* mLeft = nullptr;
    const Support* mRight = nullptr;

    bool                     mEPAIsComplete = false;

    // why in the fuck is this a list
    List< EPATriangle >      mEPATriangles;
    int                      mEPATriangleCount = 4;
    EPATriangle              mEPAClosest;
    CompoundSupport          mEPAClosestSupportPoint;
    v3                       mEPALeftPoint = {};
    v3                       mEPALeftNormal = {};
    bool                     mEPABarycentricFucked = true;
    float                    mEPAPenetrationDist = 0;
    int                      mEPAIteration = 0;

  };



}

