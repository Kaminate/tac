#pragma once

#include "common/math/tacVector3.h"
#include "common/containers/tacVector.h"

struct TacBlendState;
struct TacCBuffer;
struct TacDepthBuffer;
struct TacDepthState;
struct TacErrors;
struct TacFontStuff;
struct TacIndexBuffer;
struct TacRasterizerState;
struct TacRenderer;
struct TacSamplerState;
struct TacShader;
struct TacTexture;
struct TacUI2DCommonData;
struct TacUI2DDrawCall;
struct TacUI2DDrawData;
struct TacUI2DState;
struct TacUI2DVertex;
struct TacVertexBuffer;
struct TacVertexFormat;

struct TacDefaultVertexColor
{
  v3 mPosition;
  v3 mColor;
};

struct TacDebug3DCommonData
{
  ~TacDebug3DCommonData();
  void Init( TacErrors& errors );

  TacRenderer* mRenderer = nullptr;

  TacBlendState* mAlphaBlendState = nullptr;
  TacCBuffer* mCBufferPerFrame = nullptr;
  TacCBuffer* mCBufferPerObject = nullptr;
  TacDepthState* mDepthLess = nullptr;
  TacRasterizerState* mRasterizerStateNoCull = nullptr;
  TacShader* m3DVertexColorShader = nullptr;
  TacVertexBuffer* mDebugLineVB = nullptr;
  TacVertexFormat* mVertexColorFormat = nullptr;
  int mDebugDrawVertMax;
};

struct TacDebugDrawAABB
{
  static TacDebugDrawAABB FromMinMax( v3 mini, v3 maxi );
  static TacDebugDrawAABB FromPosExtents( v3 pos, v3 extents );
  v3 mMini = {};
  v3 mMaxi = {};
};

struct TacDebug3DDrawData
{
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
  TacVector< TacDefaultVertexColor > mDebugDrawVerts;
};

