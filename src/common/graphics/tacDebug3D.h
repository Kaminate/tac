
#pragma once

#include "src/common/math/tacVector3.h"
#include "src/common/containers/tacVector.h"
#include "src/common/graphics/tacRenderer.h"
namespace Tac
{

  struct BlendState;
  struct CBuffer;
  struct DefaultCBufferPerFrame;
  struct DepthBuffer;
  struct DepthState;
  struct Errors;
  struct FontStuff;
  struct IndexBuffer;
  struct RasterizerState;
  struct Renderer;
  struct SamplerState;
  struct Shader;
  struct Texture;
  struct UI2DCommonData;
  struct UI2DDrawCall;
  struct UI2DDrawData;
  struct UI2DState;
  struct UI2DVertex;
  struct VertexBuffer;
  struct RenderView;

  struct DefaultVertexColor
  {
    v3 mPosition;
    v3 mColor;
  };

  struct Debug3DCommonData
  {
    void                          Uninit();
    void                          Init( Errors& );
    static Debug3DCommonData*     Instance;
    Render::BlendStateHandle      mAlphaBlendState;
    Render::ConstantBufferHandle  mCBufferPerFrame;
    Render::DepthStateHandle      mDepthLess;
    Render::RasterizerStateHandle mRasterizerStateNoCull;
    Render::ShaderHandle          m3DVertexColorShader;
    Render::VertexFormatHandle    mVertexColorFormat;
  };

  struct Debug3DDrawData
  {
    ~Debug3DDrawData();
    void DebugDrawLine( v3 p0, v3 p1, v3 color0, v3 color1 );
    void DebugDrawLine( v3 p0, v3 p1, v3 color = { 1, 1, 1 } );
    void DebugDrawCircle( v3 p0, v3 dir, float rad, v3 color = { 1, 1, 1 } );
    void DebugDrawSphere( v3 origin, float radius, v3 color = { 1, 1, 1 } );
    void DebugDrawCapsule( v3 p0, v3 p1, float radius, v3 color = { 1, 1, 1 } );
    void DebugDrawHemisphere( v3 pos, v3 dir, float radius, v3 color = { 1, 1, 1 } );
    void DebugDrawCylinder( v3 p0, v3 p1, float radius, v3 color = { 1, 1, 1 } );
    void DebugDrawGrid( v3 color = { 1, 1, 1 } );
    void DebugDrawArrow( v3 from, v3 to, v3 color = { 1, 1, 1 } );
    void DebugDrawOBB( v3 pos, v3 extents, v3 eulerAnglesRad, v3 color = { 1, 1, 1 } );
    void DebugDrawAABB( v3 mini, v3 maxi, v3 color = { 1, 1, 1 } );
    void DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color0, v3 color1, v3 color2 );
    void DebugDrawTriangle( v3 p0, v3 p1, v3 p2, v3 color = v3( 1, 1, 1 ) );
    void DrawToTexture( Errors&,
                        const DefaultCBufferPerFrame*,
                        RenderView* );
    Vector< DefaultVertexColor > mDebugDrawVerts;
    Render::VertexBufferHandle   mVerts;
    int                          mCapacity = 0;
  };

  extern Debug3DCommonData gDebug3DCommonData;


}

