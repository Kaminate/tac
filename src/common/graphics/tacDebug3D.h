#pragma once

#include "src/common/math/tacVector3.h"
#include "src/common/containers/tacVector.h"
#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct BlendState;
  struct CBuffer;
  struct Camera;
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
  struct VertexBuffer;

  struct DefaultVertexColor
  {
    v3 mPosition;
    v3 mColor;
  };

  void Debug3DCommonDataInit( Errors& );
  void Debug3DCommonDataUninit();

  struct Debug3DDrawData
  {
    ~Debug3DDrawData();
    void DebugDraw3DLine( v3 p0, v3 p1, v3 color0, v3 color1 );
    void DebugDraw3DLine( v3 p0, v3 p1, v3 color = { 1, 1, 1 } );
    void DebugDraw3DCircle( v3 p0, v3 dir, float rad, v3 color = { 1, 1, 1 } );
    void DebugDraw3DSphere( v3 origin, float radius, v3 color = { 1, 1, 1 } );
    void DebugDraw3DCapsule( v3 p0, v3 p1, float radius, v3 color = { 1, 1, 1 } );
    void DebugDraw3DHemisphere( v3 pos, v3 dir, float radius, v3 color = { 1, 1, 1 } );
    void DebugDraw3DCylinder( v3 p0, v3 p1, float radius, v3 color = { 1, 1, 1 } );
    void DebugDraw3DGrid( v3 color = { 1, 1, 1 } );
    void DebugDraw3DArrow( v3 from, v3 to, v3 color = { 1, 1, 1 } );
    void DebugDraw3DOBB( v3 pos, v3 extents, v3 eulerAnglesRad, v3 color = { 1, 1, 1 } );
    void DebugDraw3DAABB( v3 mini, v3 maxi, v3 color = { 1, 1, 1 } );
    void DebugDraw3DAABB( v3 mini, v3 maxi, v3 miniColor, v3 maxiColor );
    void DebugDraw3DTriangle( v3 p0, v3 p1, v3 p2, v3 color0, v3 color1, v3 color2 );
    void DebugDraw3DTriangle( v3 p0, v3 p1, v3 p2, v3 color = v3( 1, 1, 1 ) );
    void DebugDraw3DToTexture( const Render::ViewHandle,
                               const Camera*,
                               const int viewWidth,
                               const int viewHeight,
                               Errors& );
    Vector< DefaultVertexColor > mDebugDrawVerts;
    Render::VertexBufferHandle   mVerts;
    int                          mCapacity = 0;
  };

}

