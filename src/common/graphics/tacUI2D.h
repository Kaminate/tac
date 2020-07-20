// Draws 2d shit to a texture ( such as the screen )
// like shapes and text and images and shit.

#pragma once

#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/math/tacMatrix3.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacVector4.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacLocalization.h"
#include "src/common/tacMemory.h"

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
  struct UI2DDrawData;
  struct UI2DState;
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
    static UI2DCommonData* Instance;
    UI2DCommonData();
    ~UI2DCommonData();
    void Init( Errors& errors );

    Render::TextureHandle m1x1White;
    Render::VertexFormatHandle mFormat;
    Render::ShaderHandle mShader;
    Render::ShaderHandle m2DTextShader;
    Render::DepthStateHandle mDepthState;
    Render::BlendStateHandle mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle mSamplerState;
    Render::ConstantBufferHandle mPerFrame;
    Render::ConstantBufferHandle mPerObj;
  };

  struct UI2DState
  {
    void Translate( v2 pos );
    void Translate( float x, float y );
    void Draw2DBox( float width,
                    float height,
                    v4 color = { 1, 1, 1, 1 },
                    Render::TextureHandle texture = Render::TextureHandle() );

    void Draw2DText( Language mDefaultLanguage,
                     int fontSize,
                     StringView text,
                     float* heightBetweenBaselines,
                     v4 color,
                     Errors& errors );

    m3 mTransform = m3::Identity();

    UI2DDrawData* mUI2DDrawData = nullptr;
  };

  struct UI2DDrawCall
  {
    int mIVertexStart = 0;
    int mVertexCount = 0;
    int mIIndexStart = 0;
    int mIndexCount = 0;

    Render::ShaderHandle mShader;
    //const Texture* mTexture = nullptr;
    Render::TextureHandle mTexture;
    DefaultCBufferPerObject mUniformSource;

    //void CopyUniform( const void* bytes, int byteCount );

    //template< typename T>
    //void CopyUniform( T& t ) { CopyUniform( &t, sizeof( T ) ); }
  };

  struct ImGuiRect;

  // why does this class exist
  struct UI2DDrawData
  {
    UI2DDrawData();
    ~UI2DDrawData();
    void DrawToTexture( int w, int h, Render::ViewId viewId, Errors& errors );

    UI2DState* PushState();
    void PopState();

    v2 CalculateTextSize( StringView text, int fontSize );
    v2 CalculateTextSize( const Vector< Codepoint >& codepoints, int fontSize );
    v2 CalculateTextSize( const Codepoint* codepoints, int codepointCount, int fontSize );
    void AddText( v2 textPos, int fontSize, StringView utf8, v4 color, const ImGuiRect* clipRect );
    void AddBox( v2 mini, v2 maxi, v4 color, const Render::TextureHandle texture, const ImGuiRect* clipRect );
    void AddLine( v2 p0, v2 p1, float radius, v4 color );
    //void AddPolyFill( const Vector< v2 >& points, v4 color );

    Vector< UI2DVertex > mDefaultVertex2Ds;
    Vector< UI2DIndex > mDefaultIndex2Ds;
    Vector< UI2DDrawCall > mDrawCall2Ds;
    Vector< UI2DState > mStates;


    // Hmm... this class shouldnt have both the mDefaultVertex2Ds and the mVertexBufferHandle
    // it's too much responsibilty for 1 class.
    // 
    // instead, it should be broken up into 2 classes.
    // the first class generates mDefaultVertex2Ds
    // the second class interfaces with the renderer.

    int mVertexCapacity = 0;
    int mIndexCapacity = 0;

    Render::VertexBufferHandle mVertexBufferHandle;
    Render::IndexBufferHandle mIndexBufferHandle;

    //RenderView* mRenderView = nullptr;
  };

}
