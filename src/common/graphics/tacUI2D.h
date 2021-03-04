// Draws 2d shit to a texture ( such as the screen )
// like shapes and text and images and shit.

#pragma once

#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/math/tacMatrix3.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector4.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacLocalization.h"
#include "src/common/tacMemory.h"
#include "src/common/tacSpan.h"

namespace Tac
{
  struct BlendState;
  struct CBuffer;
  struct DepthState;
  struct FontStuff;
  struct IndexBuffer;
  struct RasterizerState;
  struct RenderView;
  struct SamplerState;
  struct Shader;
  struct UI2DCommonData;
  struct UI2DDrawCall;
  struct VertexBuffer;
  struct VertexFormat;

  typedef uint16_t UI2DIndex; // formerly DefaultIndex2D

  struct UI2DVertex
  {
    v2 mPosition;
    v2 mGLTexCoord;
  };

  struct UI2DCommonData
  {
    void                          Init( Errors& );
    void                          Uninit();
    Render::TextureHandle         m1x1White;
    Render::VertexFormatHandle    mFormat;
    Render::ShaderHandle          mShader;
    Render::ShaderHandle          m2DTextShader;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
    Render::ConstantBufferHandle  mPerFrame;
    Render::ConstantBufferHandle  mPerObj;
  };

  extern UI2DCommonData           gUI2DCommonData;

  struct UI2DDrawCall
  {
    int                     mIVertexStart = 0;
    int                     mVertexCount = 0;
    int                     mIIndexStart = 0;
    int                     mIndexCount = 0;
    Render::ShaderHandle    mShader;
    Render::TextureHandle   mTexture;
    DefaultCBufferPerObject mUniformSource;
  };

  struct ImGuiRect;

  struct UI2DDrawGpuInterface
  {
    int                        mVertexCapacity = 0;
    int                        mIndexCapacity = 0;
    Render::VertexBufferHandle mVertexBufferHandle;
    Render::IndexBufferHandle  mIndexBufferHandle;
  };

  // why does this class exist
  struct UI2DDrawData
  {
    UI2DDrawData();
    ~UI2DDrawData();
    void                   DrawToTexture( Render::ViewHandle, int, int, Errors& );
    void                   AddText( v2 textPos, int fontSize, StringView utf8, v4 color, const ImGuiRect* );
    void                   AddBox( v2 mini, v2 maxi, v4 color, const Render::TextureHandle , const ImGuiRect* );
    void                   AddLine( v2 p0, v2 p1, float radius, v4 color );
    Vector< UI2DVertex >   mDefaultVertex2Ds;
    Vector< UI2DIndex >    mDefaultIndex2Ds;
    Vector< UI2DDrawCall > mDrawCall2Ds;
    UI2DDrawGpuInterface   gDrawInterface;
  };

  v2 CalculateTextSize( StringView text, int fontSize );
  v2 CalculateTextSize( CodepointView codepoints, int fontSize );
  v2 CalculateTextSize( const Codepoint* codepoints, int codepointCount, int fontSize );


}
