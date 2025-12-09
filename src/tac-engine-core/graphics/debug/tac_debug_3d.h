#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac
{
  struct Camera;

  struct DefaultVertexColor
  {
    v3 mPosition;
    v4 mColor;
  };

  void Debug3DCommonDataInit( Errors& );
  void Debug3DCommonDataUninit();

  struct Debug3DDrawBuffers
  {
    struct Buffer
    {
      ~Buffer();

      void DebugDraw3DToTexture( Render::IContext*,
                                 Render::TextureHandle color,
                                 Render::TextureHandle depth,
                                 const Camera*,
                                 v2i viewSize,
                                 Errors& ) const;

      Render::BufferHandle         mVtxBuf             {};
      int                          mVtxCount           {};
      int                          mVtxBufByteCapacity {};
    };

    auto Update( Render::IContext*, Span< DefaultVertexColor >, Errors& ) -> const Buffer*;

  private:

    Vector< Buffer > mBuffers;
    int              mRenderBufferIdx {};
  };

  struct Debug3DDrawData
  {
    void DebugDraw3DLine( const v3& p0, const v3& p1 );
    void DebugDraw3DLine( const v3& p0, const v3& p1, const v3& color );
    void DebugDraw3DLine( const v3& p0, const v3& p1, const v3& color0, const v3& color1 );
    void DebugDraw3DLine( const v3& p0, const v3& p1, const v4& color );
    void DebugDraw3DLine( const v3& p0, const v3& p1, const v4& color0, const v4& color1 );
    void DebugDraw3DCircle( const v3& p0, const v3& dir, float rad, const v3& color = { 1, 1, 1 } );
    void DebugDraw3DSphere( const v3& origin, float radius, const v3& color = { 1, 1, 1 } );
    void DebugDraw3DCapsule( const v3& p0, const v3& p1, float radius, const v3& color = { 1, 1, 1 } ); 
    void DebugDraw3DHemisphere( const v3& pos, const v3& dir, float radius, const v3& color = { 1, 1, 1 } );
    void DebugDraw3DCylinder( const v3& p0, const v3& p1, float radius, const v3& color = { 1, 1, 1 } ); 
    void DebugDraw3DGrid( const v3& color = { 1, 1, 1 } );
    void DebugDraw3DArrow( const v3& from, const v3& to, const v3& color = { 1, 1, 1 } );
    void DebugDraw3DOBB( const v3& pos, const v3& halfextents, const v3& eulerAnglesRad, const v3& color = { 1, 1, 1 } );
    void DebugDraw3DOBB( const v3& pos, const v3& halfextents, const m3& orientation, const v3& color = { 1, 1, 1 } ); 
    void DebugDraw3DCube( const v3& pos, float fullwidth, const m3& orientation, const v3& color = { 1, 1, 1 } ); 
    void DebugDraw3DAABB( const v3& mini, const v3& maxi, const v3& color = { 1, 1, 1 } );
    void DebugDraw3DAABB( const v3& mini, const v3& maxi, const v3& miniColor, const v3& maxiColor );
    void DebugDraw3DTriangle( const v3& p0, const v3& p1, const v3& p2, const v3& color0, const v3& color1, const v3& color2 );
    void DebugDraw3DTriangle( const v3& p0, const v3& p1, const v3& p2, const v3& color = { 1, 1, 1 } );
    void CopyFrom( const Debug3DDrawData& );
    void Clear();
    auto GetVerts() -> Span< DefaultVertexColor >;

  private:
    
    Vector< DefaultVertexColor > mDebugDrawVerts {};
  };

} // namespace Tac

