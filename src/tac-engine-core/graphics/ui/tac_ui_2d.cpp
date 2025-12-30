#include "tac_ui_2d.h" // self-inc

#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_math_unit_test_helper.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/memory/tac_memory.h"

namespace Tac
{
  struct PerFrameType
  {
    m4    mOrthoProj         {};
    float mSDFOnEdge         {};
    float mSDFPixelDistScale {};
  };

  struct PerObjectType
  {
    v4  mColor{};
    u32 mType{};
  };

  struct UI2DCopyHelper
  {
    using GetDrawElementBytes = void* ( * )( const UI2DDrawData* );
    using GetDrawElementCount = int ( * )( const UI2DDrawData* );

    void Copy( Render::IContext* renderContext, Errors& errors ) const
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

      const int srcTotByteCount{ mSrcTotElemCount * mSizeOfElem };
      if( !srcTotByteCount )
        return;

      if( !mDstBuffer.IsValid() || mDstBufferByteCount < srcTotByteCount )
      {
        renderDevice->DestroyBuffer( mDstBuffer );
        const Render::CreateBufferParams createBufferParams
        {
          .mByteCount    { srcTotByteCount },
          .mStride       { mSizeOfElem },
          .mUsage        { Render::Usage::Dynamic },
          .mBinding      { mBinding },
          .mGpuBufferFmt { mTexFmt },
          .mOptionalName { mBufName },
        };
        TAC_CALL( mDstBuffer = renderDevice->CreateBuffer( createBufferParams, errors ) );
        mDstBufferByteCount = srcTotByteCount;
      }

      const int drawCount{ mDraws.size() };
      Render::UpdateBufferParams* updateBufferParams{
        ( Render::UpdateBufferParams* )FrameMemoryAllocate(
          drawCount * sizeof( Render::UpdateBufferParams ) ) };

      Span< const Render::UpdateBufferParams > updates( updateBufferParams, drawCount );

      int byteOffset {};
      for( const UI2DDrawData* pDrawData : mDraws )
      {
        const int srcByteCount { mGetDrawElementCount( pDrawData ) * mSizeOfElem };

        *updateBufferParams++ = Render::UpdateBufferParams
        {
          .mSrcBytes      { mGetDrawElementBytes( pDrawData ) },
          .mSrcByteCount  { srcByteCount },
          .mDstByteOffset { byteOffset },
        };

        byteOffset += srcByteCount;
      }

      TAC_CALL( renderContext->UpdateBuffer( mDstBuffer, updates, errors ) );
    }

    Render::BufferHandle& mDstBuffer;
    int&                  mDstBufferByteCount;
    int                   mSizeOfElem;
    int                   mSrcTotElemCount;
    GetDrawElementBytes   mGetDrawElementBytes;
    GetDrawElementCount   mGetDrawElementCount;
    StringView            mBufName;
    Span< UI2DDrawData* > mDraws;
    Render::TexFmt        mTexFmt  { Render::TexFmt::kUnknown };
    Render::Binding       mBinding { Render::Binding::None };
  };

  struct Element
  {
    Render::PipelineHandle mPipeline{};
    Render::TexFmt         mTexFmt{};
    Render::IShaderVar*    mShaderImage{};
    Render::IShaderVar*    mShaderSampler{};
    Render::IShaderVar*    mShaderPerObject{};
    Render::IShaderVar*    mShaderPerFrame{};
  };

  static Render::SamplerHandle             mSampler;
  static Vector< Element >                 mElements;
  static Render::ProgramHandle             mProgram;
  static Render::BufferHandle              mPerObject;
  static Render::BufferHandle              mPerFrame;
  static Render::TextureHandle             m1x1White;
  static Render::ProgramHandle             mShader;
  static Render::ProgramHandle             m2DTextShader;
  static const Render::BlendState          sPremultipliedAlphaBlendState
  {
    .mSrcRGB   { Render::BlendConstants::One },
    .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
    .mBlendRGB { Render::BlendMode::Add },

    // do these 3 even matter?
    .mSrcA     { Render::BlendConstants::One },
    .mDstA     { Render::BlendConstants::Zero },
    .mBlendA   { Render::BlendMode::Add },
  };
  static const Render::DepthState          sDepthState
  {
    .mDepthTest  {},
    .mDepthWrite {},
    .mDepthFunc  { Render::DepthFunc::Less },
  };
  static const Render::RasterizerState     sRasterizerState
  {
    .mFillMode              { Render::FillMode::Solid },
    .mCullMode              { Render::CullMode::None },
    .mFrontCounterClockwise { true },
    .mMultisample           {},
  };
  static const Render::VertexDeclaration   sPosDecl
  {
    .mAttribute         { Render::Attribute::Position },
    .mFormat            { Render::VertexAttributeFormat::GetVector2() },
    .mAlignedByteOffset { TAC_OFFSET_OF( UI2DVertex, mPosition ) },
  };
  static const Render::VertexDeclaration   sUVDecl
  {
    .mAttribute         { Render::Attribute::Texcoord },
    .mFormat            { Render::VertexAttributeFormat::GetVector2() },
    .mAlignedByteOffset { TAC_OFFSET_OF( UI2DVertex, mGLTexCoord ) },
  };

  static auto GetElement( Render::TexFmt texFmt, Errors& errors ) -> Element
  {
    for( Element& element : mElements )
      if( element.mTexFmt == texFmt )
        return element;

    Render::VertexDeclarations vtxDecls;
    vtxDecls.push_back( sPosDecl );
    vtxDecls.push_back( sUVDecl );

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL_RET( const Render::PipelineHandle pipeline{ renderDevice->CreatePipeline(
      Render::PipelineParams
      {
        .mProgram         { mProgram },
        .mBlendState      { sPremultipliedAlphaBlendState },
        .mDepthState      { sDepthState },
        .mRasterizerState { sRasterizerState },
        .mRTVColorFmts    { texFmt },
        .mVtxDecls        { vtxDecls },
      }, errors ) } );
    Render::IShaderVar* shaderImage{ renderDevice->GetShaderVariable( pipeline, "image" ) };
    Render::IShaderVar* shaderSampler{ renderDevice->GetShaderVariable( pipeline, "linearSampler" ) };
    Render::IShaderVar* shaderPerObject{ renderDevice->GetShaderVariable( pipeline, "perObject" ) };
    Render::IShaderVar* shaderPerFrame{ renderDevice->GetShaderVariable( pipeline, "perFrame" ) };
    shaderPerFrame->SetResource( mPerFrame );
    shaderPerObject->SetResource( mPerObject );
    shaderSampler->SetResource( mSampler );
    mElements.push_back(
      Element
      {
        .mPipeline        { pipeline },
        .mTexFmt          { texFmt },
        .mShaderImage     { shaderImage },
        .mShaderSampler   { shaderSampler },
        .mShaderPerObject { shaderPerObject },
        .mShaderPerFrame  { shaderPerFrame },
      } );
    return mElements.back();
  }

  static void UpdatePerFrame( Render::IContext* context, v2i windowSize, Errors& errors )
  {
    const m4 proj{ OrthographicUIMatrix( ( float )windowSize.x, ( float )windowSize.y ) };
    const PerFrameType perFrame
    {
      .mOrthoProj         { proj },
      .mSDFOnEdge         { FontApi::GetSDFOnEdgeValue() },
      .mSDFPixelDistScale { FontApi::GetSDFPixelDistScale() },
    };
    const Render::UpdateBufferParams updatePerFrame
    {
      .mSrcBytes     { &perFrame },
      .mSrcByteCount { sizeof( PerFrameType ) },
    };
    TAC_CALL( context->UpdateBuffer( mPerFrame, &updatePerFrame, errors ) );
  }

  static void UpdatePerObject( Render::IContext* context,
                               const UI2DDrawCall& uidrawCall,
                               Errors& errors )
  {

    const u32 drawType{
      [ & ]()
      {
        if( uidrawCall.mType == UI2DDrawCall::Type::kImage )
          return ( u32 )0;
        if( uidrawCall.mType == UI2DDrawCall::Type::kText )
          return ( u32 )1;
        TAC_ASSERT_INVALID_CODE_PATH;
        return ( u32 )0;
      }( )
    };

    const PerObjectType perObject
    {
      .mColor { uidrawCall.mColor },
      .mType  { drawType },
    };

    const Render::UpdateBufferParams updatePerObj
    {
      .mSrcBytes     { &perObject },
      .mSrcByteCount { sizeof( PerObjectType ) },
    };

    TAC_CALL( context->UpdateBuffer( mPerObject, &updatePerObj, errors ) );
  }

#if TAC_IS_DEBUG_MODE()

  static m4 OrthographicUIMatrix2( const float w, const float h ) {
    // mRenderView->mViewportRect.mViewportPixelWidthIncreasingRight?
    const float sx { 2.0f / w };
    const float sy { 2.0f / h };
    const m4 projectionPieces[]
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
    m4 projection { m4::Identity() };
    for( const m4& projectionPiece : projectionPieces )
      projection = projectionPiece * projection;
    return projection;
  }

  static void UnitTest( const m4 m, const v2 in, const v2 out )
  {
    const v4 in4( in, 0, 1 );
    const v4 out4( out, 0, 1 );
    const v4 actual { m * in4 };
    const float dist { Distance( actual, out4 ) };
    TAC_ASSERT( dist < 0.01f );
  }

  static void TestOrthoFn( m4( *mtxFn )( float, float ) )
  {
    const float w { 400 };
    const float h { 300 };
    const m4 m { mtxFn( w, h ) };
    UnitTest( m, { 0, 0 }, { -1, 1 } );
    UnitTest( m, { w, 0 }, { 1, 1 } );
    UnitTest( m, { 0, h }, { -1, -1 } );
    UnitTest( m, { w, h }, { 1, -1 } );
    UnitTest( m, { w / 2, h / 2 }, { 0, 0 } );
  }

  static void UnitTest()
  {
    TestOrthoFn( OrthographicUIMatrix );
    TestOrthoFn( OrthographicUIMatrix2 );
    const float w { 400 };
    const float h { 300 };
    const m4 m0 { OrthographicUIMatrix( w, h ) };
    const m4 m1 { OrthographicUIMatrix2( w, h ) };
    AssertAboutEqual( m0, m1 );

    {
      v4 test1( 0, 0, 0, 1 );
      v4 test1prime { m0 * test1 };
      v4 test1primeexpected( -1, 1, 0, 1 );
      AssertAboutEqual( test1prime, test1primeexpected );

      v4 test2( (float)w, (float)h, 0, 1 );
      v4 test2prime { m0 * test2 };
      v4 test2primeexpected( 1, -1, 0, 1 );
      AssertAboutEqual( test2prime, test2primeexpected );
    }
  }
#endif



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
    Render::DebugGroup::Iterator debugGroupIterator { mDebugGroupStack.IterateBegin() };
    if( mVtxs.size() && mIdxs.size() )
    {
      UpdateDrawInterface( this, &gDrawInterface, errors );
      Render::BufferHandle& mBufferHandle { gDrawInterface.mBufferHandle };
      Render::BufferHandle& mBufferHandle { gDrawInterface.mBufferHandle };

      OrthographicUIMatrixUnitTest();

      const GameTime elapsedSeconds { GameTimer::GetElapsedTime() };
      const Render::DefaultCBufferPerFrame perFrameData
      {
        .mView              { m4::Identity() },
        .mProjection        { OrthographicUIMatrix( ( float )w, ( float )h ) },
        .mSecModTau         { ( float )Fmod( elapsedSeconds.mSeconds, 6.2831853 ) },
        .mSDFOnEdge         { FontApi::GetSDFOnEdgeValue() },
        .mSDFPixelDistScale { FontApi::GetSDFPixelDistScale() },
      };

      const Render::BufferHandle hPerFrame { Render::DefaultCBufferPerFrame::Handle };
      const int perFrameSize { sizeof( Render::DefaultCBufferPerFrame ) };
      Render::UpdateConstantBuffer( hPerFrame, &perFrameData, perFrameSize, TAC_STACK_FRAME );


      for( const UI2DDrawCall& uidrawCall : mDrawCall2Ds )
      {
        mDebugGroupStack.IterateElement( debugGroupIterator,
                                         uidrawCall.mDebugGroupIndex,
                                         uidrawCall.mStackFrame );

        const Render::TextureHandle texture{ uidrawCall.mTexture.IsValid() ?
          uidrawCall.mTexture :
          gUI2DCommonData.m1x1White };


        const Render::BufferHandle hPerObj { Render::DefaultCBufferPerObject::Handle };
        const int perObjSize { sizeof( Render::DefaultCBufferPerObject ) };
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

  //void UI2DDrawData::DrawToTexture2( Render::IContext*,
  //                                   Render::TextureHandle,
  //                                   UI2DDrawGpuInterface* ,
  //                                   Span< UI2DDrawData > ,
  //                                   Errors& )
  //{
      //UpdateDrawInterface( this, &gDrawInterface, errors );
  //}

  // -----------------------------------------------------------------------------------------------

  //void                   UI2DDrawData::PushDebugGroup( const StringView prefix,
  //                                                     const StringView suffix )
  //{
  //  const String debugGroup{ prefix + "(" + suffix + ")" };
  //  mDebugGroupStack.Push( debugGroup );
  //}

  void UI2DDrawData::PushDebugGroup( const StringView name )
  {
    mDebugGroupStack.Push( name );
  }

  void UI2DDrawData::PopDebugGroup()
  {
    mDebugGroupStack.Pop();
  }

  bool UI2DDrawData::empty() const
  {
    return mDrawCall2Ds.empty();
  }

  void UI2DDrawData::clear()
  {
    mVtxs.clear();
    mIdxs.clear();
    mDrawCall2Ds.clear();
    mDebugGroupStack.clear();
  }

  void UI2DDrawData::AddDrawCall( const UI2DDrawCall& drawCall, const StackFrame& stackFrame )
  {
    UI2DDrawCall modified { drawCall };
    modified.mStackFrame = stackFrame;
    modified.mDebugGroupIndex = mDebugGroupStack.GetInfo();

    mDrawCall2Ds.resize( mDrawCall2Ds.size() + 1, modified );
    //mDrawCall2Ds.push_back( modified ); <-- Why not just do this instead?
  }

  void UI2DDrawData::AddBoxOutline( const Box& box, const ImGuiRect* clipRect )
  {
    Box T{ box }; T.mMaxi = v2( box.mMaxi.x, box.mMini.y + 1); AddBox( T, clipRect );
    Box B{ box }; B.mMini = v2( box.mMini.x, box.mMaxi.y - 1); AddBox( B, clipRect );
    Box L{ box }; L.mMaxi = v2( box.mMini.x + 1, box.mMaxi.y); AddBox( L, clipRect );
    Box R{ box }; R.mMini = v2( box.mMaxi.x - 1, box.mMini.y); AddBox( R, clipRect );
  }

  void UI2DDrawData::AddBox( const Box& box, const ImGuiRect* clipRect )
  {
    const v2& mini { box.mMini };
    const v2& maxi { box.mMaxi };

    TAC_ASSERT( mini.x <= maxi.x );
    TAC_ASSERT( mini.y <= maxi.y );

    v2 clippedMini { mini };
    v2 clippedMaxi { maxi };
    if( clipRect )
    {
      clippedMini = { Max( mini.x, clipRect->mMini.x ), Max( mini.y, clipRect->mMini.y ) };
      clippedMaxi = { Min( maxi.x, clipRect->mMaxi.x ), Min( maxi.y, clipRect->mMaxi.y ) };
    }

    if( clippedMini.x >= clippedMaxi.x || clippedMini.y >= clippedMaxi.y )
      return;

    const int oldVtxCount { mVtxs.size() };
    const int oldIdxCount { mIdxs.size() };
    const int idxCount { 6 };
    mIdxs.resize( oldIdxCount + idxCount );
    UI2DIndex* pIdx { mIdxs.data() + oldIdxCount };
    const UI2DIndex idxs[] { 0, 1, 2, 0, 2, 3 };
    for( UI2DIndex idx : idxs )
      *pIdx++ = (UI2DIndex)oldVtxCount + idx;

    const int vtxCount { 4 };
    mVtxs.resize( oldVtxCount + 4 );
    UI2DVertex* pVtx = mVtxs.data() + oldVtxCount;
    *pVtx++ = { .mPosition  { clippedMini.x, clippedMini.y }, .mGLTexCoord  { 0, 1 } };
    *pVtx++ = { .mPosition  { clippedMini.x, clippedMaxi.y }, .mGLTexCoord  { 0, 0 } };
    *pVtx++ = { .mPosition  { clippedMaxi.x, clippedMaxi.y }, .mGLTexCoord  { 1, 0 } };
    *pVtx++ = { .mPosition  { clippedMaxi.x, clippedMini.y }, .mGLTexCoord  { 1, 1 } };
    const v4 color { Render::PremultipliedAlpha::From_sRGB_linearAlpha( box.mColor ).mColor };
    TAC_ASSERT( oldVtxCount + vtxCount == mVtxs.size() );
    TAC_ASSERT( oldIdxCount + idxCount == mIdxs.size() );
    AddDrawCall(
      UI2DDrawCall
      {
        .mIVertexStart  { oldVtxCount },
        .mVertexCount   { vtxCount },
        .mIIndexStart   { oldIdxCount },
        .mIndexCount    { idxCount },
        .mType          { UI2DDrawCall::Type::kImage },
        .mTexture       { box.mTextureHandle },
        .mColor         { color },
      }, TAC_STACK_FRAME );
  }

  void UI2DDrawData::AddLine( const Line& line)
  {
    const v2& p0 { line.mP0 };
    const v2& p1 { line.mP1 };
    const float radius { line.mLineRadius };

    // This function creates a long thin rectangle to act as a line
    const v2 dp { p1 - p0 };
    const float quadrance { dp.Quadrance() };
    if( dp.Quadrance() < 0.01f )
      return;

    const float length { Sqrt( quadrance ) };
    const v2 dp_hat { dp / length };
    const v2 dp_hat_ccw_scaled { v2( -dp_hat.y, dp_hat.x ) * radius };

    const int iVert { mVtxs.size() };
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

    const v4 color{ Render::PremultipliedAlpha::From_sRGB_linearAlpha( line.mColor ).mColor };

    const UI2DDrawCall drawCall
    {
      .mIVertexStart  { iVert },
      .mVertexCount   { 4 },
      .mIIndexStart   { iIndex },
      .mIndexCount    { 6 },
      .mType          { UI2DDrawCall::Type::kImage },
      .mColor         { color },
    };

    AddDrawCall( drawCall, TAC_STACK_FRAME );
  }

  void UI2DDrawData::AddText( const Text& text, const ImGuiRect* clipRect )
  {
#if TAC_FONT_ENABLED()
    const v2& textPos { text.mPos };
    const float fontSize { text.mFontSize };
    const StringView utf8 { text.mUtf8 };
    const v4& color { text.mColor };

    if( utf8.empty() || fontSize <= 0 )
      return;

    const CodepointView codepoints{ UTF8ToCodepointView( utf8 ) };

    Language defaultLanguage { Language::English };
    const FontDims* fontFile { FontApi::GetLanguageFontDims( defaultLanguage ) };

    const float fontSizeRelativeScale { fontSize / TextPxHeight };
    const float fontSizeScale { fontFile->mScale * fontSizeRelativeScale };

    // do i really need to floor everything? does alignment on pixel grid matter?
    const float ascentPx{ ( float )( int )( fontFile->mUnscaledAscent * fontSizeScale ) };
    //const float descentPx{ ( float )( int )( fontFile->mUnscaledDescent * fontSizeScale ) };
    //const float linegapPx{ ( float )( int )( fontFile->mUnscaledLinegap * fontSizeScale ) };
    const float unscaledLineSpacing
    {
      fontFile->mUnscaledAscent -
      fontFile->mUnscaledDescent +
      fontFile->mUnscaledLinegap
    };
    const float lineSpacingPx { (float)(int)(unscaledLineSpacing * fontSizeScale) };

    static v2 glyphMin{};
    static v2 glyphMax{};


    v2 baselineCursorPos { textPos + v2( 0, ascentPx ) };

    const int oldStrIdxCount { mIdxs.size() };
    const int oldStrVtxCount { mVtxs.size() };

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

      const FontAtlasCell* fontAtlasCell {
        FontApi::GetFontAtlasCell( defaultLanguage, codepoint ) };

      if( !fontAtlasCell )
        continue;

      TAC_ON_DESTRUCT(
        // Even if we don't draw a glyph, we still need to advance the cursor,
        // For example if our current codepoint represents whitespace
        baselineCursorPos.x += fontAtlasCell->mGlyphMetrics.mUnscaledAdvanceWidth * fontSizeScale;
      );

      if( !fontAtlasCell->mGlyphMetrics. mSDFWidth || !fontAtlasCell->mGlyphMetrics.mSDFHeight )
        continue;

      // the mSDFxOffset stuff seem to come from stbtt_GetGlyphBitmapBoxSubpixel and account for the
      // sdf padding
      //
      // idk how the sdf offsets take into account the stbtt_GetFontVMetrics ascent descent stuff

      v2 sdfOffset( ( float )fontAtlasCell->mGlyphMetrics.mSDFxOffset,
                          ( float )fontAtlasCell->mGlyphMetrics.mSDFyOffset );
      v2 sdfSize( ( float )fontAtlasCell->mGlyphMetrics.mSDFWidth,
                        ( float )fontAtlasCell->mGlyphMetrics.mSDFHeight );

      // 2025-12-02 fixme: this offset is causing text to not be centered inside buttons...
      glyphMin = baselineCursorPos + fontSizeRelativeScale * ( sdfOffset );
      glyphMax = baselineCursorPos + fontSizeRelativeScale * ( sdfOffset + sdfSize );

      const ImGuiRect glyphRect { ImGuiRect::FromMinMax( glyphMin, glyphMax ) };
      

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

      const float GLMinU{ fontAtlasCell->mFontCellUVs.mMinDXTexCoord.x };
      const float GLMaxU{ fontAtlasCell->mFontCellUVs.mMaxDXTexCoord.x };
      const float GLMinV{ 1.0f - fontAtlasCell->mFontCellUVs.mMaxDXTexCoord.y };
      const float GLMaxV{ 1.0f - fontAtlasCell->mFontCellUVs.mMinDXTexCoord.y };

      const int idxCount { 6 };
      const int vtxCount { 4 };

      const UI2DVertex TL
      {
        .mPosition    { glyphMin.x, glyphMin.y },
        .mGLTexCoord  { GLMinU, GLMaxV }
      };
      const UI2DVertex BL
      {
        .mPosition    { glyphMin.x, glyphMax.y },
        .mGLTexCoord  { GLMinU, GLMinV }
      };
      const UI2DVertex BR
      {
        .mPosition    { glyphMax.x, glyphMax.y },
        .mGLTexCoord  { GLMaxU, GLMinV }
      };
      const UI2DVertex TR
      {
        .mPosition    { glyphMax.x, glyphMin.y },
        .mGLTexCoord  { GLMaxU, GLMaxV }
      };
      const UI2DVertex vtxs[]  { TL, BL, BR, TR };
      static_assert( vtxCount == TAC_ARRAY_SIZE( vtxs ) );

      const int oldCharVtxCount { mVtxs.size() };
      const int oldCharIdxCount { mIdxs.size() };
      const UI2DIndex idxs[]  { 0, 1, 2, 0, 2, 3 };
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

    const int newStrIdxCount { mIdxs.size() };
    const int newStrVtxCount { mVtxs.size() };

    const int strIdxCount { newStrIdxCount - oldStrIdxCount };
    const int strVtxCount { newStrVtxCount - oldStrVtxCount };

    // Everything has been clipped
    if( !strIdxCount && !strVtxCount )
      return;

    const Render::PremultipliedAlpha colorPremultiplied {
      Render::PremultipliedAlpha::From_sRGB_linearAlpha( color ) };

    const Render::TextureHandle texture { FontApi::GetAtlasTextureHandle() };

    const UI2DDrawCall drawCall
    {
      .mIVertexStart  { oldStrVtxCount },
      .mVertexCount   { strVtxCount },
      .mIIndexStart   { oldStrIdxCount },
      .mIndexCount    { strIdxCount },
      .mType          { UI2DDrawCall::Type::kText },
      .mTexture       { texture },
      .mColor         { colorPremultiplied.mColor },
    };

    AddDrawCall( drawCall, TAC_STACK_FRAME );
#endif
  }

  void UI2DRenderData::DebugDraw2DToTexture( Render::IContext* renderContext,
                                             const Span< UI2DDrawData* > drawDatas,
                                             const Render::TextureHandle texture,
                                             const Render::TexFmt texFmt,
                                             const v2i textureSize,
                                             Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const int nFrames{ Render::RenderApi::GetMaxGPUFrameCount() };
    const int iFrame{ ( int )( Render::RenderApi::GetCurrentRenderFrameIndex() % ( u64 )nFrames ) };

    mVBs.resize( nFrames );
    mVBByteCounts.resize( nFrames );
    mIBs.resize( nFrames );
    mIBByteCounts.resize( nFrames );

    Render::BufferHandle& vB{ mVBs[ iFrame ] };
    Render::BufferHandle& iB{ mIBs[ iFrame ] };
    int& vBByteCount{ mVBByteCounts[ iFrame ] };
    int& iBByteCount{ mIBByteCounts[ iFrame ] };


    int combinedVtxCount{};
    int combinedIdxCount{};
    for( const UI2DDrawData* drawData : drawDatas )
    {
      combinedVtxCount += drawData->mVtxs.size();
      combinedIdxCount += drawData->mIdxs.size();
    }

    // combine draw data
    {
      const ShortFixedString vtxBufName{ ShortFixedString::Concat(
        "UI2D_vtx_buf " ,
        "(frame ", ToString( iFrame ), ")"
      ) };

      const ShortFixedString idxBufName{ ShortFixedString::Concat(
        String( "UI2D_idx_buf " ) ,
        "(frame ", ToString( iFrame ), ")"
      ) };

      auto getVtxBytes{ []( const UI2DDrawData* dd ) { return ( void* )dd->mVtxs.data(); } };
      auto getVtxCount{ []( const UI2DDrawData* dd ) { return dd->mVtxs.size(); } };
      const UI2DCopyHelper vtxCopyHelper
      {
        .mDstBuffer           { vB },
        .mDstBufferByteCount  { vBByteCount },
        .mSizeOfElem          { sizeof( UI2DVertex ) },
        .mSrcTotElemCount     { combinedVtxCount },
        .mGetDrawElementBytes { getVtxBytes },
        .mGetDrawElementCount { getVtxCount },
        .mBufName             { vtxBufName },
        .mDraws               { drawDatas },
        .mBinding             { Render::Binding::VertexBuffer },
      };

      auto getIdxBytes{ []( const UI2DDrawData* dd ) { return ( void* )dd->mIdxs.data(); } };
      auto getIdxCount{ []( const UI2DDrawData* dd ) { return dd->mIdxs.size(); } };

      const UI2DCopyHelper idxCopyHelper
      {
        .mDstBuffer           { iB },
        .mDstBufferByteCount  { iBByteCount },
        .mSizeOfElem          { sizeof( UI2DIndex ) },
        .mSrcTotElemCount     { combinedIdxCount },
        .mGetDrawElementBytes { getIdxBytes },
        .mGetDrawElementCount { getIdxCount },
        .mBufName             { idxBufName },
        .mDraws               { drawDatas },
        .mTexFmt              { Render::TexFmt::kR16_uint },
        .mBinding             { Render::Binding::IndexBuffer },
      };

      TAC_CALL( vtxCopyHelper.Copy( renderContext, errors ) );
      TAC_CALL( idxCopyHelper.Copy( renderContext, errors ) );
    }

    const Element& element{ GetElement( texFmt, errors ) };
    const ShortFixedString renderGroupStr{ "UI2D" };

    const Render::Targets renderTargets { .mColors { texture }, };

    UpdatePerFrame( renderContext, textureSize, errors );

    renderContext->SetVertexBuffer( vB );
    renderContext->SetIndexBuffer( iB );
    renderContext->SetPipeline( element.mPipeline );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( textureSize );
    renderContext->SetScissor( textureSize );
    renderContext->DebugEventBegin( renderGroupStr );
    renderContext->DebugMarker( "hello hello" );

    for( UI2DDrawData* drawData : drawDatas ) //  simDraws->mDrawData )
    {
      Render::DebugGroup::Stack& debugGroupStack{ drawData->mDebugGroupStack };
      debugGroupStack.AssertNodeHeights();

      static Render::DebugGroup::Iterator debugGroupIterator;
      debugGroupIterator.Reset( renderContext );

      TAC_ASSERT( debugGroupStack.GetInfo() == Render::DebugGroup::NullNodeIndex );

      for( const UI2DDrawCall& uidrawCall : drawData->mDrawCall2Ds )
      {
        debugGroupStack.IterateElement( debugGroupIterator, uidrawCall.mDebugGroupIndex );
        const Render::TextureHandle textureResource{ uidrawCall.mTexture.IsValid()
          ? uidrawCall.mTexture
          : m1x1White };

        element.mShaderImage->SetResource( textureResource );

        UpdatePerObject( renderContext, uidrawCall, errors );
        renderContext->CommitShaderVariables();
        renderContext->Draw(
          Render::DrawArgs
          {
            .mIndexCount { uidrawCall.mIndexCount },
            .mStartIndex { uidrawCall.mIIndexStart },
          } );
      }

      debugGroupStack.IterateEnd( debugGroupIterator );

      //for( ImGuiWindow* window : ImGuiGlobals::mAllWindows )
      //  window->mDrawData->mDebugGroupStack.clear();

    }

    renderContext->DebugEventEnd();
  }

} // namespace Tac

// Converts from UI space to NDC space
auto Tac::OrthographicUIMatrix( const float w, const float h ) -> m4
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

  const float L {};
  const float R { w };
  const float T {};
  const float B { h };
  const float sX { 2 / ( R - L ) };
  const float sY { 2 / ( T - B ) };
  const float tX { ( R + L ) / ( L - R ) };
  const float tY { ( T + B ) / ( B - T ) };
  return
  {
    sX, 0, 0, tX,
    0, sY, 0, tY,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
}


auto Tac::CalculateTextSize( const StringView text, const float fontSize ) -> v2
{
  const CodepointView codepoints{ UTF8ToCodepointView( text ) };
  return CalculateTextSize( codepoints, fontSize );
}

auto Tac::CalculateTextSize( const CodepointView codepoints, const float fontSize ) -> v2
{
  return CalculateTextSize( codepoints.data(),
                            codepoints.size(),
                            fontSize );
}

// Uhh yeah so this function doesnt account for SDF padding (its old)
// (still useful as a guideline?)
auto Tac::CalculateTextSize( const Codepoint* codepoints,
                             const int codepointCount,
                             const float fontSize ) -> v2
{
#if TAC_FONT_ENABLED()

  //float unscaledLineWidthMax = 0; // max width of all lines
  //float unscaledLineWidthCur = 0;
  float xUnscaled {};
  float xUnscaledMax {};

  Language defaultLanguage { Language::English };
  auto fontFile { FontApi::GetLanguageFontDims( defaultLanguage ) };

  int lineCount { 1 };

  //auto AccountForLine = [ &unscaledLineWidthMax, &unscaledLineWidth, &xUnscaled ]()
  //{
  //  unscaledLineWidthMax = Max( unscaledLineWidthMax, unscaledLineWidth );
  //  // if the string ends with a space ( ' ' )
  //  // ( bitmap width is 0, advance width is nonzero )
  //  unscaledLineWidthMax = Max( unscaledLineWidthMax, xUnscaled );
  //};

  for( int iCodepoint{}; iCodepoint < codepointCount; ++iCodepoint )
  {
    Codepoint codepoint { codepoints[ iCodepoint ] };
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

    const FontAtlasCell* fontAtlasCell{
      FontApi::GetFontAtlasCell( defaultLanguage, codepoint ) };

    if( !fontAtlasCell )
      continue;

    xUnscaled += fontAtlasCell->mGlyphMetrics.mUnscaledAdvanceWidth;
    xUnscaledMax = Max( xUnscaledMax, xUnscaled );
    //unscaledLineWidthCur = Max( unscaledLineWidthCur, xUnscaled );
    //unscaledLineWidthMax = Max( unscaledLineWidthMax, unscaledLineWidthCur );
  }

  //AccountForLine();

  const float fontSizeScale { fontFile->mScale * ( fontSize / TextPxHeight ) };

  const int gapCount { lineCount - 1 };
  const float unscaledLineHeight { fontFile->mUnscaledAscent - fontFile->mUnscaledDescent };

  const float yUnscaledMax { lineCount * unscaledLineHeight + gapCount * fontFile->mUnscaledLinegap };

  const v2 unscaledTextSize( xUnscaledMax, yUnscaledMax );
  const v2 scaledTextSize { unscaledTextSize * fontSizeScale };
  return scaledTextSize;
#else
  return {};
#endif
}

void Tac::UI2DCommonDataInit( Errors& errors )
{ 

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    mSampler = renderDevice->CreateSampler(
      Render::CreateSamplerParams
    {
      .mFilter { Render::Filter::Linear },
      .mName   { "imgui linear sampler" },
    } );

    TAC_CALL( mProgram = renderDevice->CreateProgram(
      Render::ProgramParams{ .mInputs { "ImGui" }, },
      errors ) );

    TAC_CALL( mPerFrame = renderDevice->CreateBuffer(
      Render::CreateBufferParams
    {
      .mByteCount     { sizeof( PerFrameType ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "UI2D_per_frame" },
    }, errors ) );

    TAC_CALL( mPerObject = renderDevice->CreateBuffer(
      Render::CreateBufferParams 
    {
      .mByteCount     { sizeof( PerObjectType ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "UI2D_per_object" },
    }, errors ) );


    const u8 data[] { 255, 255, 255, 255 };
    const Render::CreateTextureParams::Subresource subresource
    {
      .mBytes { data },
      .mPitch { 4 },
    };
    const Render::CreateTextureParams createTextureParams
    {
      .mImage        {
        Render::Image
        {
          .mWidth  { 1 },
          .mHeight { 1 },
          .mFormat { Render::TexFmt::kRGBA8_unorm },
        } },
      .mMipCount     { 1 },
      .mSubresources { &subresource },
      .mBinding      { Render::Binding::ShaderResource },
      .mOptionalName { "1x1white" },
    };
    TAC_CALL( m1x1White = renderDevice->CreateTexture( createTextureParams, errors ) );


#if TAC_TEMPORARILY_DISABLED()
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const u8 data[] { 255, 255, 255, 255 };
    const Render::CreateTextureParams textureData
    {
      .mImage
      {
        .mWidth { 1 },
        .mHeight { 1 },
        .mFormat
        {
          .mElementCount { 4 },
          .mPerElementByteCount { 1 },
          .mPerElementDataType = Render::GraphicsType::unorm,
        },
      },
      .mPitch { 1 },
      .mImageBytes { data },
      .mBinding { Render::Binding::ShaderResource },
    };
    m1x1White = Render::CreateTexture( textureData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( m1x1White, "1x1white" );

    const Render::ProgramParams programParams2D
    {
      .mFileStem   { "2D" },
      .mStackFrame { TAC_STACK_FRAME },
    };
    mShader = renderDevice->CreateProgram( programParams2D, errors );

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams2Dtext
    {
      .mFileStem   { "2Dtext" },
      .mStackFrame { TAC_STACK_FRAME },
    };
    m2DTextShader = renderDevice->CreateProgram( programParams2Dtext, errors );

    const Render::VertexDeclaration posData
    {
      .mAttribute { Render::Attribute::Position },
      .mTextureFormat = Render::Format
      {
        .mElementCount        { 2 },
        .mPerElementByteCount { sizeof( float ) },
        .mPerElementDataType  { Render::GraphicsType::rea }l
      },
      .mAlignedByteOffset { TAC_OFFSET_OF( UI2DVertex, mPosition ) },
    };

    const Render::VertexDeclaration uvData
    {
      .mAttribute { Render::Attribute::Texcoord },
      .mTextureFormat
      { 
        .mElementCount        { 2 },
        .mPerElementByteCount { sizeof( float ) },
        .mPerElementDataType  { Render::GraphicsType::rea }l
      },
      .mAlignedByteOffset { TAC_OFFSET_OF( UI2DVertex, mGLTexCoord ) },
    };

    Render::VertexDeclarations decls;
    decls.push_back( posData );
    decls.push_back( uvData );

    mFormat = Render::CreateVertexFormat( decls, mShader, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mFormat, "2dVtxFmt" );

    const Render::BlendState blendStateData
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::One },
      .mDstA     { Render::BlendConstants::OneMinusSrcA },
      .mBlendA   { Render::BlendMode::Add },
    };
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mBlendState, "2dalphablend" );

    const Render::DepthState depthStateData
    {
      .mDepthTest  {},
      .mDepthWrite {},
      .mDepthFunc  { Render::DepthFunc::Less },
    };
    mDepthState = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mDepthState, "2d-no-depth" );

    const Render::RasterizerState rasterizerStateData
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mScissor               { true },
      .mMultisample           {},
    };
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerState , "2d-rast" );


    const Render::SamplerState samplerStateData
    {
      .mU      { Render::AddressMode::Clamp },
      .mV      { Render::AddressMode::Clamp },
      .mFilter { Render::Filter::Linear },
    };
    mSamplerState = Render::CreateSamplerState( samplerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerState  , "2d-samp" );
#else
    TAC_UNUSED_PARAMETER( errors );
#endif

}

void Tac::UI2DCommonDataUninit()              
{
#if TAC_TEMPORARILY_DISABLED()
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyTexture( m1x1White );
    renderDevice->DestroyVertexFormat( mFormat );
    renderDevice->DestroyProgram( mShader );
    renderDevice->DestroyProgram( m2DTextShader );
    renderDevice->DestroyDepthState( mDepthState );
    renderDevice->DestroyBlendState( mBlendState );
    renderDevice->DestroyRasterizerState( mRasterizerState );
    renderDevice->DestroySamplerState( mSamplerState );
#endif

}
