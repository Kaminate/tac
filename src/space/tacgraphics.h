#pragma once
//#include "taccommon.h"
#include "tacsystem.h"
#include "common/tacRenderer.h"
#include "common/tacLocalization.h"
//#include "graphics/tacDefaultGeometry.h"
#include <set>

struct TacFontStuff;
//struct TacSay;
//struct TacModel;
//struct TacComponent;

struct TacDebugDrawAABB
{
  static TacDebugDrawAABB FromMinMax( v3 mini, v3 maxi );
  static TacDebugDrawAABB FromPosExtents( v3 pos, v3 extents );
  v3 mMini = {};
  v3 mMaxi = {};
};

struct TacGraphics : public TacSystem
{
  TacComponent* CreateComponent( TacComponentType componentType ) override;
  void DestroyComponent( TacComponent* component ) override;
  TacSystemType GetSystemType() override { return TacSystemType::Graphics; }

  void DebugImgui() override;

  // Note about DebugDraw...() prototypes:
  //
  // When drawing a shape that contains multiple points, we should be able to specify
  // different colors for each point
  //
  // For convenience, we also offer an overload that defaults all points to white

  void DebugDrawLine( v3 p0, v3 p1, v3 color, v3 color1 );
  void DebugDrawLine( v3 p0, v3 p1, v3 color = { 1, 1, 1 } );
  void DebugDrawCircle( v3 p0, v3 dir, float rad, v3 color = { 1, 1, 1 } );
  void DebugDrawSphere( v3 origin, float radius, v3 color = { 1, 1, 1 } );
  void DebugDrawCapsule( v3 p0, v3 p1, float radius, v3 color = { 1, 1, 1 } );
  void DebugDrawHemisphere( v3 pos, v3 dir, float radius, v3 color = { 1, 1, 1 } );
  void DebugDrawCylinder( v3 p0, v3 p1, float radius, v3 color = { 1, 1, 1 } );
  void DebugDrawGrid( v3 color = { 1, 1, 1 } );
  void DebugDrawArrow( v3 from, v3 to, v3 color = { 1, 1, 1 } );
  void DebugDrawOBB( v3 pos, v3 extents, v3 eulerAnglesRad, v3 color = { 1, 1, 1 } );
  void DebugDrawAABB( TacDebugDrawAABB debugDrawAABB, v3 color = { 1, 1, 1 } );
  void DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color0, v3 color1, v3 color2 );
  void DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color = v3( 1, 1, 1 ) );

  //std::set< TacSay* > mSays;
  //std::set< TacStuff* > mStuffs;
  //std::set< TacModel* > mModels;
  TacVector< TacDefaultVertexColor > mDebugDrawVerts;
};
