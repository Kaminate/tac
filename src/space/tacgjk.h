// The Gilbert–Johnson–Keerthi algorithm computes the intersection
// of two convex polygons. We use it for physics integration and collision detection

#pragma once
#include "common/containers/tacVector.h"
#include "common/math/tacVector3.h"
#include <list>

struct TacSupport
{
  virtual v3 Support( const v3& dir ) = 0;
};

struct TacSphereSupport : public TacSupport
{
  TacSphereSupport() = default;
  TacSphereSupport( v3 origin, float radius );
  v3 Support( const v3& dir ) override;
  v3 mOrigin;
  float mRadius = 0;
};

struct TacCapsuleSupport : public TacSupport
{
  TacCapsuleSupport() = default;
  TacCapsuleSupport( v3 base, float height, float radius );
  v3 Support( const v3& dir ) override;
  v3 mBotSpherePos;
  v3 mTopSpherePos;
  float mRadius;
};

struct TacConvexPolygonSupport : public TacSupport
{
  TacConvexPolygonSupport() = default;
  TacConvexPolygonSupport( v3 obbPos, v3 obbHalfExtents, v3 obbEulerRads );
  v3 Support( const v3& dir ) override;
  TacVector< v3 > mPoints;
};

struct TacCompoundSupport
{
  v3 mDiffPt;
  v3 mLeftPoint;
};

struct TacEPATriangle
{
  TacEPATriangle() = default;
  TacEPATriangle(
    TacCompoundSupport v0,
    TacCompoundSupport v1,
    TacCompoundSupport v2,
    v3 toNormalize );
  TacEPATriangle(
    TacCompoundSupport v0,
    TacCompoundSupport v1,
    TacCompoundSupport v2 );
  v3 GetArbitraryPointOnTriangle();
  void ComputeDist();

  TacCompoundSupport mV0;
  TacCompoundSupport mV1;
  TacCompoundSupport mV2;
  v3 mNormal;
  float mPlaneDist;
};

struct TacEPAHalfEdge
{
  TacEPAHalfEdge() = default;
  TacEPAHalfEdge(
    TacCompoundSupport from,
    TacCompoundSupport to );
  TacEPAHalfEdge Reverse();
  bool operator == ( const TacEPAHalfEdge& other ) const;
  TacCompoundSupport mFrom;
  TacCompoundSupport mTo;
};

struct TacGJK
{
  TacGJK() = default;
  TacGJK( TacSupport* left, TacSupport* right );
  void EnsureCorrectTetrahedronOrientation();
  void Step();
  void EPAStep();
  TacCompoundSupport Support( const v3& dir );

  bool mIsRunning = true;
  bool mIsColliding = false;
  int mIteration = 0;
  v3 mNormalizedSearchDir = { 1, 0, 0 };
  v3 mClosestPoint = {};
  TacVector< TacCompoundSupport > mSupports;
  TacSupport* mLeft = nullptr;
  TacSupport* mRight = nullptr;

  bool mEPAIsComplete = false;
  std::list< TacEPATriangle > mEPATriangles;
  int mEPATriangleCount = 4;
  TacEPATriangle mEPAClosest;
  TacCompoundSupport mEPAClosestSupportPoint;
  v3 mEPALeftPoint = {};
  v3 mEPALeftNormal = {};
  bool mEPABarycentricFucked = true;
  float mEPAPenetrationDist = 0;
  int mEPAIteration = 0;

};

void BarycentricTriangle(
  v3 p,
  v3 tri0,
  v3 tri1,
  v3 tri2,
  bool& fucked,
  float& bary0,
  float& bary1,
  float& bary2 );
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
  float& bary3 );

