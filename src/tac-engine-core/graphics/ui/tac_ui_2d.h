// Draws 2d shit to a texture ( such as the screen )
// like shapes and text and images and shit.

#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-std-lib/containers/tac_vector.h" // Vector
//#include "tac-rhi/render3/tac_render_api.h" // Render::
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/graphics/tac_renderer_util.h" // DefaultCBufferPerObject
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-engine-core/graphics/debug/tac_debug_group.h"
#include "tac-std-lib/containers/tac_span.h"

namespace Tac
{
  using UI2DIndex = u16;

  struct UI2DVertex
  {
    v2 mPosition    {};
    v2 mGLTexCoord  {};
  };

  void UI2DCommonDataInit( Errors& );
  void UI2DCommonDataUninit();

  struct UI2DDrawCall
  {
    enum Type { kImage, kText };
    int                             mIVertexStart    {};
    int                             mVertexCount     {};
    int                             mIIndexStart     {};
    int                             mIndexCount      {};
    Type                            mType            { Type::kImage };
    Render::TextureHandle           mTexture         {};
    v4                              mColor           {};
    StackFrame                      mStackFrame      {};
    Render::DebugGroup::NodeIndex   mDebugGroupIndex { Render::DebugGroup::NullNodeIndex };
  };

  struct ImGuiRect;

  struct UI2DDrawGpuInterface
  {
    int                  mVertexCapacity     {};
    int                  mIndexCapacity      {};
    Render::BufferHandle mVertexBufferHandle {};
    Render::BufferHandle mIndexBufferHandle  {};
  };


  // why does this class exist
  struct UI2DDrawData
  {
    struct Text
    {
      v2                    mPos      {};
      float                 mFontSize {};
      StringView            mUtf8     {};
      v4                    mColor    { 1, 1, 1, 1 };
    };

    struct Box
    {
      v2                    mMini          {};
      v2                    mMaxi          {};
      v4                    mColor         { 1, 1, 1, 1 };
      Render::TextureHandle mTextureHandle {};
    };

    struct Line
    {
      v2                    mP0         {};
      v2                    mP1         {};
      float                 mLineRadius { 1.0f };
      v4                    mColor      { 1, 1, 1, 1 };
    };

    UI2DDrawData() = default;
    ~UI2DDrawData() = default;

    static void DrawToTexture2( Render::IContext*,
                                Render::TextureHandle,
                                UI2DDrawGpuInterface*,
                                Span< UI2DDrawData >,
                                Errors& );

#if 0
    void                   DrawToTexture( Render::ViewHandle, int w, int h, Errors& );
#endif

    void                   AddText( const Text&, const ImGuiRect* = nullptr );
    void                   AddBox( const Box&, const ImGuiRect* = nullptr );
    void                   AddLine( const Line& );

    void                   AddDrawCall( const UI2DDrawCall&, const StackFrame& );
    void                   PushDebugGroup( const StringView& );
    void                   PopDebugGroup();
    bool                   empty() const;
    void                   clear();

    Vector< UI2DVertex >      mVtxs;
    Vector< UI2DIndex >       mIdxs;
    Vector< UI2DDrawCall >    mDrawCall2Ds;
    Render::DebugGroup::Stack mDebugGroupStack;
  };

  Render::TextureHandle Get1x1White();

  v2                    CalculateTextSize( const StringView&, float fontSize );
  v2                    CalculateTextSize( const CodepointView&, float fontSize );
  v2                    CalculateTextSize( const Codepoint*, int, float fontSize );

  m4                    OrthographicUIMatrix( const float w, const float h );

} // namespace Tac

