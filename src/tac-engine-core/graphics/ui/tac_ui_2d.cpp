#include "tac_ui_2d.h" // self-inc

#include "tac-std-lib/containers/tac_array.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
//#include "tac-std-lib/os/tac_os.h" // deleteme
#include "tac-std-lib/math/tac_math.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-std-lib/math/tac_math_unit_test_helper.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/memory/tac_memory.h"

namespace Tac
{

  static struct UI2DCommonData
  {
    void Init( Errors& );
    void Uninit();

    Render::TextureHandle         m1x1White;
    Render::ProgramHandle          mShader;
    Render::ProgramHandle          m2DTextShader;
#if TAC_TEMPORARILY_DISABLED()
    Render::VertexFormatHandle    mFormat;
    Render::DepthStateHandle      mDepthState;
    Render::BlendStateHandle      mBlendState;
    Render::RasterizerStateHandle mRasterizerState;
    Render::SamplerStateHandle    mSamplerState;
#endif
  } gUI2DCommonData;

  static m4 OrthographicUIMatrix2( const float w, const float h )
  {
    // mRenderView->mViewportRect.mViewportPixelWidthIncreasingRight?
    const float sx = 2.0f / w;
    const float sy = 2.0f / h;
    const m4 projectionPieces[] =
    {
      // orient to bottom left
      m4( 1, 0, 0, 0,
          0, -1, 0, ( float )h,
          0, 0, 1, 0,
          0, 0, 0, 1 ),
      // convert to ndc
      m4( sx, 0, 0, -1,
          0, sy, 0, -1,
          0, 0, 1, 0,
          0, 0, 0, 1 )
    };
    m4 projection = m4::Identity();
    for( const m4& projectionPiece : projectionPieces )
      projection = projectionPiece * projection;
    return projection;
  }

  // Converts from UI space to NDC space
  m4 OrthographicUIMatrix( const float w, const float h )
  {
    // Derivation:
    //   L < x < R
    //   0 < x - L < R - L
    //   0 < ( x - L ) / ( R - L ) < 1
    //   0 < 2 ( x - L ) / ( R - L ) < 2
    //   -1 < ( 2 ( x - L ) / ( R - L ) ) - 1 < -1
    //   -1 < 2x/(R-L) + (R+L)/(L-R) < -1
    //
    // this function has the same output as OrthographicUIMatrix2(w,h)

    const float L = 0;
    const float R = w;
    const float T = 0;
    const float B = h;
    const float sX = 2 / ( R - L );
    const float sY = 2 / ( T - B );
    const float tX = ( R + L ) / ( L - R );
    const float tY = ( T + B ) / ( B - T );
    return
    {
      sX, 0, 0, tX,
      0, sY, 0, tY,
      0, 0, 1, 0,
      0, 0, 0, 1
    };
  }

  static void OrthographicUIMatrixUnitTest( const m4 m, const v2 in, const v2 out )
  {
    const v4 in4( in, 0, 1 );
    const v4 out4( out, 0, 1 );
    const v4 actual = m * in4;
    const float dist = Distance( actual, out4 );
    TAC_ASSERT( dist < 0.01f );
  }

  static void OrthographicUIMatrixUnitTest( m4( *mtxFn )( float, float ) )
  {
    const float w = 400;
    const float h = 300;
    const m4 m = mtxFn( w, h );
    OrthographicUIMatrixUnitTest( m, { 0, 0 }, { -1, 1 } );
    OrthographicUIMatrixUnitTest( m, { w, 0 }, { 1, 1 } );
    OrthographicUIMatrixUnitTest( m, { 0, h }, { -1, -1 } );
    OrthographicUIMatrixUnitTest( m, { w, h }, { 1, -1 } );
    OrthographicUIMatrixUnitTest( m, { w / 2, h / 2 }, { 0, 0 } );
  }


  static void OrthographicUIMatrixUnitTest()
  {
    OrthographicUIMatrixUnitTest( OrthographicUIMatrix );
    OrthographicUIMatrixUnitTest( OrthographicUIMatrix2 );
    const float w = 400;
    const float h = 300;
    const m4 m0 = OrthographicUIMatrix( w, h );
    const m4 m1 = OrthographicUIMatrix2( w, h );
    AssertAboutEqual( m0, m1 );

    {
      v4 test1( 0, 0, 0, 1 );
      v4 test1prime = m0 * test1;
      v4 test1primeexpected( -1, 1, 0, 1 );
      AssertAboutEqual( test1prime, test1primeexpected );

      v4 test2( (float)w, (float)h, 0, 1 );
      v4 test2prime = m0 * test2;
      v4 test2primeexpected( 1, -1, 0, 1 );
      AssertAboutEqual( test2prime, test2primeexpected );
    }
  }

#if 0
  static void UpdateDrawInterface( UI2DDrawData* drawData,
                                   UI2DDrawGpuInterface* gDrawInterface,
                                   Errors& errors )
  {
    Vector< UI2DVertex >& mDefaultVertex2Ds = drawData->mVtxs;
    Vector< UI2DIndex >& mDefaultIndex2Ds = drawData->mIdxs;

    int& mVertexCapacity = gDrawInterface->mVertexCapacity;
    int& mIndexCapacity = gDrawInterface->mIndexCapacity;
    Render::BufferHandle& mBufferHandle = gDrawInterface->mBufferHandle;
    Render::BufferHandle& mBufferHandle = gDrawInterface->mBufferHandle;


    const int vertexCount = mDefaultVertex2Ds.size();
    const int indexCount = mDefaultIndex2Ds.size();
    if( !mBufferHandle.IsValid() || mVertexCapacity < vertexCount )
    {
      if( mBufferHandle.IsValid() )
        Render::DestroyVertexBuffer( mBufferHandle, TAC_STACK_FRAME );
      mBufferHandle = Render::CreateBuffer( mDefaultVertex2Ds.size() * sizeof( UI2DVertex ),
                                                        nullptr,
                                                        sizeof( UI2DVertex ),
                                                        Render::Access::Dynamic,
                                                        TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( mBufferHandle, "ui2d-vtx-buf" );
      mVertexCapacity = vertexCount;
    }

    if( !mBufferHandle.IsValid() || mIndexCapacity < indexCount )
    {
      if( mBufferHandle.IsValid() )
        Render::DestroyIndexBuffer( mBufferHandle, TAC_STACK_FRAME );
      Render::Format format{ .mElementCount = 1,
                             .mPerElementByteCount = sizeof( UI2DIndex ),
                             .mPerElementDataType = Render::GraphicsType::uint };
      mBufferHandle = Render::CreateIndexBuffer( indexCount * sizeof( UI2DIndex ),
                                                      nullptr,
                                                      Render::Access::Dynamic,
                                                      format,
                                                      TAC_STACK_FRAME );
      Render::SetRenderObjectDebugName( mBufferHandle, "ui2d-idx-buf" );
      mIndexCapacity = indexCount;
    }


    Render::UpdateVertexBuffer( mBufferHandle,
                                mDefaultVertex2Ds.data(),
                                mDefaultVertex2Ds.size() * sizeof( UI2DVertex ),
                                TAC_STACK_FRAME );

    Render::UpdateIndexBuffer( mBufferHandle,
                               mDefaultIndex2Ds.data(),
                               mDefaultIndex2Ds.size() * sizeof( UI2DIndex ),
                               TAC_STACK_FRAME );
  }
#endif


  void UI2DCommonDataInit( Errors& errors ){ gUI2DCommonData.Init( errors ); }
  void UI2DCommonData::Init( Errors& errors )
  {
#if TAC_TEMPORARILY_DISABLED()
    const u8 data[] = { 255, 255, 255, 255 };
    const Render::CreateTextureParams textureData =
    {
      .mImage
      {
        .mWidth = 1,
        .mHeight = 1,
        .mFormat
        {
          .mElementCount = 4,
          .mPerElementByteCount = 1,
          .mPerElementDataType = Render::GraphicsType::unorm,
        },
      },
      .mPitch = 1,
      .mImageBytes = data,
      .mBinding = Render::Binding::ShaderResource,
    };
    m1x1White = Render::CreateTexture( textureData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( m1x1White, "1x1white" );

    mShader = Render::CreateShader( "2D", TAC_STACK_FRAME );
    m2DTextShader = Render::CreateShader( "2Dtext", TAC_STACK_FRAME );

    const Render::VertexDeclaration posData
    {
      .mAttribute = Render::Attribute::Position,
      .mTextureFormat = Render::Format
      {
        .mElementCount = 2,
        .mPerElementByteCount = sizeof( float ),
        .mPerElementDataType = Render::GraphicsType::real
      },
      .mAlignedByteOffset = TAC_OFFSET_OF( UI2DVertex, mPosition ),
    };

    const Render::VertexDeclaration uvData
    {
      .mAttribute = Render::Attribute::Texcoord,
      .mTextureFormat
      { 
        .mElementCount = 2,
        .mPerElementByteCount = sizeof( float ),
        .mPerElementDataType = Render::GraphicsType::real
      },
      .mAlignedByteOffset = TAC_OFFSET_OF( UI2DVertex, mGLTexCoord ),
    };

    Render::VertexDeclarations decls;
    decls.push_back( posData );
    decls.push_back( uvData );

    mFormat = Render::CreateVertexFormat( decls, mShader, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mFormat, "2dVtxFmt" );

    const Render::BlendState blendStateData
    {
      .mSrcRGB = Render::BlendConstants::One,
      .mDstRGB = Render::BlendConstants::OneMinusSrcA,
      .mBlendRGB = Render::BlendMode::Add,
      .mSrcA = Render::BlendConstants::One,
      .mDstA = Render::BlendConstants::OneMinusSrcA,
      .mBlendA = Render::BlendMode::Add,
    };
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mBlendState, "2dalphablend" );

    const Render::DepthState depthStateData
    {
      .mDepthTest = false,
      .mDepthWrite = false,
      .mDepthFunc = Render::DepthFunc::Less,
    };
    mDepthState = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mDepthState, "2d-no-depth" );

    const Render::RasterizerState rasterizerStateData
    {
      .mFillMode = Render::FillMode::Solid,
      .mCullMode = Render::CullMode::None,
      .mFrontCounterClockwise = true,
      .mScissor = true,
      .mMultisample = false,
    };
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerState , "2d-rast" );


    const Render::SamplerState samplerStateData
    {
      .mU = Render::AddressMode::Clamp,
      .mV = Render::AddressMode::Clamp,
      .mFilter = Render::Filter::Linear,
    };
    mSamplerState = Render::CreateSamplerState( samplerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerState  , "2d-samp" );
#endif
  }

  void UI2DCommonDataUninit() { gUI2DCommonData.Uninit(); }
  void UI2DCommonData::Uninit()
  {
#if TAC_TEMPORARILY_DISABLED()
    Render::DestroyTexture( m1x1White, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( mFormat, TAC_STACK_FRAME );
    Render::DestroyShader( mShader, TAC_STACK_FRAME );
    Render::DestroyShader( m2DTextShader, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
    Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerState, TAC_STACK_FRAME );
#endif
  }

#if 0

  // When this function is called, vertexes are initially defined in UI space.
  //
  //  (0, 0)          (w, 0)
  //    +--->-----------+ 
  //    |UI Space       | 
  //    v               |
  //    |               |
  //    |               |
  //    |               |
  //    |               |
  //    +---------------+
  // (0, h)            (w, h)
  //
  // The orthographicUIMatrix converts them to NDC space
  //
  // (-1, 1)          (1, 1)
  //    +-------^-------+
  //    |       |       | 
  //    |       |       |
  //    |       | NDC Space
  //    |       +------->
  //    |               |
  //    |               |
  //    |               |
  //    +---------------+
  // (-1, -1)        (1, -1)
  //
  void UI2DDrawData::DrawToTexture( const Render::ViewHandle viewHandle,
                                    int w,
                                    int h,
                                    Errors& errors )
  {
    Render::DebugGroup::Iterator debugGroupIterator = mDebugGroupStack.IterateBegin();
    if( mVtxs.size() && mIdxs.size() )
    {
      UpdateDrawInterface( this, &gDrawInterface, errors );
      Render::BufferHandle& mBufferHandle = gDrawInterface.mBufferHandle;
      Render::BufferHandle& mBufferHandle = gDrawInterface.mBufferHandle;

      OrthographicUIMatrixUnitTest();

      const Timestamp elapsedSeconds = Timestep::GetElapsedTime();
      const Render::DefaultCBufferPerFrame perFrameData
      {
        .mView = m4::Identity(),
        .mProjection = OrthographicUIMatrix( ( float )w, ( float )h ),
        .mSecModTau = ( float )Fmod( elapsedSeconds.mSeconds, 6.2831853 ),
        .mSDFOnEdge = FontApi::GetSDFOnEdgeValue(),
        .mSDFPixelDistScale = FontApi::GetSDFPixelDistScale(),
      };

      const Render::BufferHandle hPerFrame = Render::DefaultCBufferPerFrame::Handle;
      const int perFrameSize = sizeof( Render::DefaultCBufferPerFrame );
      Render::UpdateConstantBuffer( hPerFrame, &perFrameData, perFrameSize, TAC_STACK_FRAME );


      for( const UI2DDrawCall& uidrawCall : mDrawCall2Ds )
      {
        mDebugGroupStack.IterateElement( debugGroupIterator,
                                         uidrawCall.mDebugGroupIndex,
                                         uidrawCall.mStackFrame );

        const Render::TextureHandle texture = uidrawCall.mTexture.IsValid() ?
          uidrawCall.mTexture :
          gUI2DCommonData.m1x1White;


        const Render::BufferHandle hPerObj = Render::DefaultCBufferPerObject::Handle;
        const int perObjSize = sizeof( Render::DefaultCBufferPerObject );
        Render::UpdateConstantBuffer( hPerObj,
                                      &uidrawCall.mUniformSource,
                                      perObjSize,
                                      uidrawCall.mStackFrame );
        Render::SetBlendState( gUI2DCommonData.mBlendState );
        Render::SetRasterizerState( gUI2DCommonData.mRasterizerState );
        Render::SetSamplerState( { gUI2DCommonData.mSamplerState } );
        Render::SetDepthState( gUI2DCommonData.mDepthState );
        Render::SetVertexFormat( gUI2DCommonData.mFormat );
        Render::SetVertexBuffer( mBufferHandle, uidrawCall.mIVertexStart, uidrawCall.mVertexCount );
        Render::SetIndexBuffer( mBufferHandle, uidrawCall.mIIndexStart, uidrawCall.mIndexCount );
        Render::SetTexture( { texture } );
        Render::SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
        Render::SetShader( uidrawCall.mShader );
        Render::Submit( viewHandle, uidrawCall.mStackFrame );
      }
    }

    mDebugGroupStack.IterateEnd( debugGroupIterator, TAC_STACK_FRAME );
    mDebugGroupStack = {};
    mDrawCall2Ds.resize( 0 );
    mVtxs.resize( 0 );
    mIdxs.resize( 0 );
  }
#endif

  void UI2DDrawData::DrawToTexture2( Render::IContext*,
                                     Render::TextureHandle,
                                     UI2DDrawGpuInterface* gpuBuffers,
                                     Span< UI2DDrawData > drawDatas,
                                     Errors& errors )
  {
      //UpdateDrawInterface( this, &gDrawInterface, errors );
  }

  // -----------------------------------------------------------------------------------------------

  void                   UI2DDrawData::PushDebugGroup( const StringView& prefix,
                                                       const StringView& suffix )
  {
    const int n = 30;
    String& debugGroup = mDebugGroupStack.Push();
    debugGroup += prefix.substr( 0, Min( prefix.size(), n ) );
    debugGroup += prefix.size() > n ? "..." : "";
    debugGroup += ' ';
    debugGroup += suffix.substr( 0, Min( suffix.size(), n ) );
    debugGroup += suffix.size() > n ? "..." : "";
  }

  void                   UI2DDrawData::PushDebugGroup( const StringView& name )
  {
    mDebugGroupStack.Push( name );
  }

  void                   UI2DDrawData::PopDebugGroup()
  {
    mDebugGroupStack.Pop();
  }

  void UI2DDrawData::AddDrawCall( const UI2DDrawCall& drawCall, const StackFrame& stackFrame )
  {
    UI2DDrawCall modified = drawCall;
    modified.mStackFrame = stackFrame;
    modified.mDebugGroupIndex = mDebugGroupStack.GetInfo();

    mDrawCall2Ds.resize( mDrawCall2Ds.size() + 1, modified );
  }

  void UI2DDrawData::AddBox( const Box& box, const ImGuiRect* clipRect )
  {
    const v2& mini = box.mMini;
    const v2& maxi = box.mMaxi;
    const v4& color = box.mColor;
    const Render::TextureHandle texture = box.mTextureHandle;

    v2 clippedMini = mini;
    v2 clippedMaxi = maxi;
    if( clipRect )
    {
      clippedMini = { Max( mini.x, clipRect->mMini.x ), Max( mini.y, clipRect->mMini.y ) };
      clippedMaxi = { Min( maxi.x, clipRect->mMaxi.x ), Min( maxi.y, clipRect->mMaxi.y ) };
    }

    if( clippedMini.x >= clippedMaxi.x || clippedMini.y >= clippedMaxi.y )
      return;

    const int oldVtxCount = mVtxs.size();
    const int oldIdxCount = mIdxs.size();
    const int idxCount = 6;
    mIdxs.resize( oldIdxCount + idxCount );
    UI2DIndex* pIdx = mIdxs.data() + oldIdxCount;
    const UI2DIndex idxs[] = { 0, 1, 2, 0, 2, 3 };
    for( UI2DIndex idx : idxs )
      *pIdx++ = (UI2DIndex)oldVtxCount + idx;

    const int vtxCount = 4;
    mVtxs.resize( oldVtxCount + 4 );
    UI2DVertex* pVtx = mVtxs.data() + oldVtxCount;
    *pVtx++ = { .mPosition = { clippedMini.x, clippedMini.y }, .mGLTexCoord = { 0, 1 } };
    *pVtx++ = { .mPosition = { clippedMini.x, clippedMaxi.y }, .mGLTexCoord = { 0, 0 } };
    *pVtx++ = { .mPosition = { clippedMaxi.x, clippedMaxi.y }, .mGLTexCoord = { 1, 0 } };
    *pVtx++ = { .mPosition = { clippedMaxi.x, clippedMini.y }, .mGLTexCoord = { 1, 1 } };

    const Render::DefaultCBufferPerObject perObjectData =
    {
      .World = m4::Identity(),
      .Color = Render::PremultipliedAlpha::From_sRGB_linearAlpha( color ),
    };

    TAC_ASSERT( oldVtxCount + vtxCount == mVtxs.size() );
    TAC_ASSERT( oldIdxCount + idxCount == mIdxs.size() );

    const UI2DDrawCall drawCall
    {
      .mIVertexStart = oldVtxCount,
      .mVertexCount = vtxCount,
      .mIIndexStart = oldIdxCount,
      .mIndexCount = idxCount,
      .mShader = gUI2DCommonData.mShader,
      .mTexture = texture,
      .mUniformSource = perObjectData,
    };

    AddDrawCall( drawCall, TAC_STACK_FRAME );
  }


  void UI2DDrawData::AddLine( const Line& line)
  {
    const v2& p0 = line.mP0;
    const v2& p1 = line.mP1;
    const float radius = line.mLineRadius;
    const v4& color = line.mColor;

    // This function creates a long thin rectangle to act as a line
    const v2 dp = p1 - p0;
    const float quadrance = dp.Quadrance();
    if( dp.Quadrance() < 0.01f )
      return;

    const float length = Sqrt( quadrance );
    const v2 dp_hat = dp / length;
    const v2 dp_hat_ccw_scaled = v2( -dp_hat.y, dp_hat.x ) * radius;

    const int iVert = mVtxs.size();
    mVtxs.resize( iVert + 4 );
    mVtxs[ iVert + 0 ] = UI2DVertex( p0 + dp_hat_ccw_scaled );
    mVtxs[ iVert + 1 ] = UI2DVertex( p0 - dp_hat_ccw_scaled );
    mVtxs[ iVert + 2 ] = UI2DVertex( p1 + dp_hat_ccw_scaled );
    mVtxs[ iVert + 3 ] = UI2DVertex( p1 - dp_hat_ccw_scaled );

    const int iIndex = mIdxs.size();
    mIdxs.resize( iIndex + 6 );
    mIdxs[ iIndex + 0 ] = ( UI2DIndex )iVert + 0;
    mIdxs[ iIndex + 1 ] = ( UI2DIndex )iVert + 1;
    mIdxs[ iIndex + 2 ] = ( UI2DIndex )iVert + 2;
    mIdxs[ iIndex + 3 ] = ( UI2DIndex )iVert + 1;
    mIdxs[ iIndex + 4 ] = ( UI2DIndex )iVert + 3;
    mIdxs[ iIndex + 5 ] = ( UI2DIndex )iVert + 2;

    const Render::DefaultCBufferPerObject perObjectData =
    {
      .World = m4::Identity(),
      .Color = Render::PremultipliedAlpha::From_sRGB_linearAlpha(color),
    };

    const UI2DDrawCall drawCall
    {
      .mIVertexStart = iVert,
      .mVertexCount = 4,
      .mIIndexStart = iIndex,
      .mIndexCount = 6,
      .mShader = gUI2DCommonData.mShader,
      .mUniformSource = perObjectData,
    };

    AddDrawCall( drawCall, TAC_STACK_FRAME );
  }


  void UI2DDrawData::AddText( const Text& text, const ImGuiRect* clipRect )
  {
    const v2& textPos = text.mPos;
    const float fontSize = text.mFontSize;
    const StringView& utf8 = text.mUtf8;
    const v4& color = text.mColor;

    if( utf8.empty() )
      return;

    CodepointView codepoints = UTF8ToCodepoints( utf8 );

    Language defaultLanguage = Language::English;
    auto fontFile = FontApi::GetLanguageFontDims( defaultLanguage );

    const float fontSizeRelativeScale = fontSize / TextPxHeight;
    const float fontSizeScale = fontFile->mScale * fontSizeRelativeScale;

    // do i really need to floor everything? does alignment on pixel grid matter?
    const float ascentPx = (float)(int)(fontFile->mUnscaledAscent * fontSizeScale);
    const float descentPx = (float)(int)(fontFile->mUnscaledDescent * fontSizeScale);
    const float linegapPx = (float)(int)(fontFile->mUnscaledLinegap * fontSizeScale);
    const float unscaledLineSpacing
      = fontFile->mUnscaledAscent
      - fontFile->mUnscaledDescent
      + fontFile->mUnscaledLinegap;
    const float lineSpacingPx = (float)(int)(unscaledLineSpacing * fontSizeScale);

    static v2 glyphMin{};
    static v2 glyphMax{};


    v2 baselineCursorPos = textPos + v2( 0, ascentPx );

    const int oldStrIdxCount = mIdxs.size();
    const int oldStrVtxCount = mVtxs.size();

    for( Codepoint codepoint : codepoints )
    {
      if( !codepoint || codepoint == ( Codepoint )'\r' )
        continue;

      if( codepoint == ( Codepoint )'\n' )
      {
        baselineCursorPos.x = textPos.x;
        baselineCursorPos.y += lineSpacingPx;
        continue;
      }

      const FontAtlasCell* fontAtlasCell = FontApi::GetFontAtlasCell( defaultLanguage, codepoint );
      if( !fontAtlasCell )
        continue;

      TAC_ON_DESTRUCT(
        // Even if we don't draw a glyph, we still need to advance the cursor,
        // For example if our current codepoint represents whitespace
        baselineCursorPos.x += fontAtlasCell->mUnscaledAdvanceWidth * fontSizeScale;
      );

      if( !fontAtlasCell->mSDFWidth || !fontAtlasCell->mSDFHeight )
        continue;

      // the mSDFxOffset stuff seem to come from stbtt_GetGlyphBitmapBoxSubpixel and account for the
      // sdf padding
      //
      // idk how the sdf offsets take into account the stbtt_GetFontVMetrics ascent descent stuff

      v2 sdfOffset( ( float )fontAtlasCell->mSDFxOffset, ( float )fontAtlasCell->mSDFyOffset );
      v2 sdfSize( ( float )fontAtlasCell->mSDFWidth, ( float )fontAtlasCell->mSDFHeight );

      //v2 baselinePos( xPxCursor, yPxBaseline );

      glyphMin = baselineCursorPos + fontSizeRelativeScale * (sdfOffset);
      glyphMax = baselineCursorPos + fontSizeRelativeScale * (sdfOffset + sdfSize);

      const ImGuiRect glyphRect = ImGuiRect::FromMinMax( glyphMin, glyphMax );
      

      // Coordinate system reminder:
      //  UI Space (0,0) to (w,h) of target
      //   +-->x
      //   |
      //   v
      //   y

      if( clipRect && !clipRect->Overlaps( glyphRect ) )
        continue;

      TAC_ASSERT( glyphMax.x > glyphMin.x );
      TAC_ASSERT( glyphMax.y > glyphMin.y );

      // todo: compute clipped uvs


      // Position coordinate system
      //
      // 0----3-----> x
      // |\023|
      // | \  |
      // |  \ |
      // |012\|
      // 1----2
      // |
      // v
      // y

      const float GLMinU = fontAtlasCell->mMinDXTexCoord.x;
      const float GLMaxU = fontAtlasCell->mMaxDXTexCoord.x;
      const float GLMinV = 1.0f - fontAtlasCell->mMaxDXTexCoord.y;
      const float GLMaxV = 1.0f - fontAtlasCell->mMinDXTexCoord.y;

      const int idxCount = 6;
      const int vtxCount = 4;

      const UI2DVertex TL{ .mPosition = { glyphMin.x, glyphMin.y }, .mGLTexCoord = { GLMinU, GLMaxV } };
      const UI2DVertex BL{ .mPosition = { glyphMin.x, glyphMax.y }, .mGLTexCoord = { GLMinU, GLMinV } };
      const UI2DVertex BR{ .mPosition = { glyphMax.x, glyphMax.y }, .mGLTexCoord = { GLMaxU, GLMinV } };
      const UI2DVertex TR{ .mPosition = { glyphMax.x, glyphMin.y }, .mGLTexCoord = { GLMaxU, GLMaxV } };
      const UI2DVertex vtxs[] = { TL, BL, BR, TR };
      static_assert( vtxCount == TAC_ARRAY_SIZE( vtxs ) );

      const int oldCharVtxCount = mVtxs.size();
      const int oldCharIdxCount = mIdxs.size();
      const UI2DIndex idxs[] = { 0, 1, 2, 0, 2, 3 };
      static_assert( idxCount == TAC_ARRAY_SIZE( idxs ) );

      mIdxs.resize( oldCharIdxCount + idxCount );
      auto pIdx = &mIdxs[ oldCharIdxCount ];
      for( const UI2DIndex& idx : idxs )
        *pIdx++ = idx + ( UI2DIndex )oldCharVtxCount;


      mVtxs.resize( oldCharVtxCount + vtxCount );
      auto pVtx = &mVtxs[ oldCharVtxCount ];
      for( const auto& vtx : vtxs )
        *pVtx++ = vtx;

    }

    const int newStrIdxCount = mIdxs.size();
    const int newStrVtxCount = mVtxs.size();

    const int strIdxCount = newStrIdxCount - oldStrIdxCount;
    const int strVtxCount = newStrVtxCount - oldStrVtxCount;

    // Everything has been clipped
    if( !strIdxCount && !strVtxCount )
      return;

    const Render::DefaultCBufferPerObject perObjectData =
    {
      .World = m4::Identity(),
      .Color = Render::PremultipliedAlpha::From_sRGB_linearAlpha( color ),
    };

    const Render::TextureHandle texture = FontApi::GetAtlasTextureHandle();

    const UI2DDrawCall drawCall
    {
      .mIVertexStart = oldStrVtxCount,
      .mVertexCount = strVtxCount,
      .mIIndexStart = oldStrIdxCount,
      .mIndexCount = strIdxCount,
      .mShader = gUI2DCommonData.m2DTextShader,
      .mTexture = texture,
      .mUniformSource = perObjectData,
    };

    AddDrawCall( drawCall, TAC_STACK_FRAME );
  }



  v2 CalculateTextSize( const StringView& text, const float fontSize )
  {
    const CodepointView codepoints = UTF8ToCodepoints( text );
    return CalculateTextSize( codepoints, fontSize );
  }

  v2 CalculateTextSize( const CodepointView& codepoints,
                        const float fontSize )
  {
    return CalculateTextSize( codepoints.data(),
                              codepoints.size(),
                              fontSize );
  }

  // Uhh yeah so this function doesnt account for SDF padding (its old)
  // (still useful as a guideline?)
  v2 CalculateTextSize( const Codepoint* codepoints,
                        const int codepointCount,
                        const float fontSize )
  {

    //float unscaledLineWidthMax = 0; // max width of all lines
    //float unscaledLineWidthCur = 0;
    float xUnscaled = 0;
    float xUnscaledMax = 0;

    Language defaultLanguage = Language::English;
    auto fontFile = FontApi::GetLanguageFontDims( defaultLanguage );

    int lineCount = 1;

    //auto AccountForLine = [ &unscaledLineWidthMax, &unscaledLineWidth, &xUnscaled ]()
    //{
    //  unscaledLineWidthMax = Max( unscaledLineWidthMax, unscaledLineWidth );
    //  // if the string ends with a space ( ' ' )
    //  // ( bitmap width is 0, advance width is nonzero )
    //  unscaledLineWidthMax = Max( unscaledLineWidthMax, xUnscaled );
    //};

    for( int iCodepoint = 0; iCodepoint < codepointCount; ++iCodepoint )
    {
      Codepoint codepoint = codepoints[ iCodepoint ];
      if( !codepoint )
        continue;

      // First check for newlines
      if( IsAsciiCharacter( codepoint ) )
      {
        if( codepoint == '\r' )
          continue; // ignore '\r'

        if( codepoint == '\n' )
        {
          //AccountForLine();
          //unscaledLineWidthCur = 0;
          xUnscaled = 0;
          lineCount++;
        }
      }

      const FontAtlasCell* fontAtlasCell = FontApi::GetFontAtlasCell( defaultLanguage, codepoint );
      if( !fontAtlasCell )
        continue;

      xUnscaled += fontAtlasCell->mUnscaledAdvanceWidth;
      xUnscaledMax = Max( xUnscaledMax, xUnscaled );
      //unscaledLineWidthCur = Max( unscaledLineWidthCur, xUnscaled );
      //unscaledLineWidthMax = Max( unscaledLineWidthMax, unscaledLineWidthCur );
    }

    //AccountForLine();

    float fontSizeScale = fontFile->mScale * ( fontSize / TextPxHeight );

    int gapCount = lineCount - 1;
    float unscaledLineHeight = fontFile->mUnscaledAscent - fontFile->mUnscaledDescent;

    float yUnscaledMax = lineCount * unscaledLineHeight + gapCount * fontFile->mUnscaledLinegap;

    const v2 unscaledTextSize( xUnscaledMax, yUnscaledMax );
    const v2 scaledTextSize = unscaledTextSize * fontSizeScale;
    return scaledTextSize;
  }

  Render::TextureHandle Get1x1White() { return gUI2DCommonData.m1x1White; }
} // namespace Tac
