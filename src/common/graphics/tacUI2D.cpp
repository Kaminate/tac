#include "src/common/graphics/tacUI2D.h"

#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/containers/tacArray.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/math/tacMath.h"

#include <cmath>

namespace Tac
{
  UI2DCommonData gUI2DCommonData;

  static m4 OrthographicUIMatrix2( const float w, const float h )
  {
    // mRenderView->mViewportRect.mViewportPixelWidthIncreasingRight?
    float sx = 2.0f / w;
    float sy = 2.0f / h;
    m4 projectionPieces [] =
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
  static m4 OrthographicUIMatrix( const float w, const float h )
  {
    const float L = 0;
    const float R = w;
    const float T = 0;
    const float B = h;
    // Derivation:
    // L < x < R
    // 0 < x - L < R - L
    // 0 < ( x - L ) / ( R - L ) < 1
    // 0 < 2 ( x - L ) / ( R - L ) < 2
    // -1 < ( 2 ( x - L ) / ( R - L ) ) - 1 < -1
    // -1 < 2x/(R-L) + (R+L)/(L-R) < -1
    return
    {
      2 / ( R - L ), 0, 0, ( R + L ) / ( L - R ),
      0, 2 / ( T - B ), 0, ( T + B ) / ( B - T ),
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
    for( int i = 0; i < 16; ++i )
    {
      const float diff = Abs( m0[ i ] - m1[ i ] );
      TAC_ASSERT( diff - 0.01f );
    }
  }


  void UI2DCommonData::Init( Errors& errors )
  {
    uint8_t data[] = { 255, 255, 255, 255 };
    Render::TexSpec textureData = {};
    textureData.mImage.mWidth = 1;
    textureData.mImage.mHeight = 1;
    textureData.mPitch = 1;
    textureData.mImage.mFormat.mElementCount = 4;
    textureData.mImage.mFormat.mPerElementByteCount = 1;
    textureData.mImage.mFormat.mPerElementDataType = Render::GraphicsType::unorm;
    textureData.mImageBytes = data;
    textureData.mBinding = Render::Binding::ShaderResource;
    m1x1White = Render::CreateTexture( textureData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mPerFrame = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerFrame ),
                                              DefaultCBufferPerFrame::shaderRegister,
                                              TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mPerObj = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerObject ),
                                            DefaultCBufferPerObject::shaderRegister,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mShader = Render::CreateShader( Render::ShaderSource::FromPath( "2D" ),
                                    Render::ConstantBuffers{ mPerFrame, mPerObj },
                                    TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    m2DTextShader = Render::CreateShader( Render::ShaderSource::FromPath( "2Dtext" ),
                                          Render::ConstantBuffers{ mPerFrame, mPerObj },
                                          TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::VertexDeclaration posData;
    posData.mAlignedByteOffset = TAC_OFFSET_OF( UI2DVertex, mPosition );
    posData.mAttribute = Render::Attribute::Position;
    posData.mTextureFormat = formatv2;

    Render::VertexDeclaration uvData;
    uvData.mAlignedByteOffset = TAC_OFFSET_OF( UI2DVertex, mGLTexCoord );
    uvData.mAttribute = Render::Attribute::Texcoord;
    uvData.mTextureFormat = formatv2;

    //VertexDeclarations vertexDeclarations;
    //vertexDeclarations.AddVertexDeclaration( posData );
    //vertexDeclarations.AddVertexDeclaration( uvData );
    //mFormat = Render::CreateVertexFormat( vertexDeclarations,
    mFormat = Render::CreateVertexFormat( Render::VertexDeclarations{posData,uvData},
                                          mShader,
                                          TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::BlendState blendStateData;
    blendStateData.mSrcRGB = Render::BlendConstants::One;
    blendStateData.mDstRGB = Render::BlendConstants::OneMinusSrcA;
    blendStateData.mBlendRGB = Render::BlendMode::Add;
    blendStateData.mSrcA = Render::BlendConstants::One;
    blendStateData.mDstA = Render::BlendConstants::OneMinusSrcA;
    blendStateData.mBlendA = Render::BlendMode::Add;
    mBlendState = Render::CreateBlendState( blendStateData,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::DepthState depthStateData;
    depthStateData.mDepthTest = false;
    depthStateData.mDepthWrite = false;
    depthStateData.mDepthFunc = Render::DepthFunc::Less;
    mDepthState = Render::CreateDepthState( depthStateData,
                                            TAC_STACK_FRAME );

    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = Render::CullMode::None;
    rasterizerStateData.mFillMode = Render::FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData,
                                                      TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Render::Filter::Linear;
    mSamplerState = Render::CreateSamplerState( samplerStateData,
                                                TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }

  void UI2DCommonData::Uninit()
  {
    {
      Render::DestroyTexture( m1x1White, TAC_STACK_FRAME );
      Render::DestroyVertexFormat( mFormat, TAC_STACK_FRAME );
      Render::DestroyShader( mShader, TAC_STACK_FRAME );
      Render::DestroyShader( m2DTextShader, TAC_STACK_FRAME );
      Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
      Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
      Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
      Render::DestroySamplerState( mSamplerState, TAC_STACK_FRAME );
      Render::DestroyConstantBuffer( mPerFrame, TAC_STACK_FRAME );
      Render::DestroyConstantBuffer( mPerObj, TAC_STACK_FRAME );
    }
  }
  UI2DDrawData::UI2DDrawData()
  {


  }

  UI2DDrawData::~UI2DDrawData()
  {
    //if( mVertexBufferHandle.IsValid() )
    //  Render::DestroyVertexBuffer( mVertexBufferHandle, TAC_STACK_FRAME );
    //if( mIndexBufferHandle.IsValid() )
    //  Render::DestroyIndexBuffer( mIndexBufferHandle, TAC_STACK_FRAME );
  }



  static void UpdateDrawInterface( UI2DDrawData* drawData,
                                   UI2DDrawGpuInterface* gDrawInterface,
                                   Errors& errors )
  {
    Vector< UI2DVertex >& mDefaultVertex2Ds = drawData->mDefaultVertex2Ds;
    Vector< UI2DIndex >& mDefaultIndex2Ds = drawData->mDefaultIndex2Ds;

    int& mVertexCapacity = gDrawInterface->mVertexCapacity;
    int& mIndexCapacity = gDrawInterface->mIndexCapacity;
    Render::VertexBufferHandle& mVertexBufferHandle = gDrawInterface->mVertexBufferHandle;
    Render::IndexBufferHandle& mIndexBufferHandle = gDrawInterface->mIndexBufferHandle;


    const int vertexCount = mDefaultVertex2Ds.size();
    const int indexCount = mDefaultIndex2Ds.size();
    if( !mVertexBufferHandle.IsValid() || mVertexCapacity < vertexCount )
    {
      if( mVertexBufferHandle.IsValid() )
        Render::DestroyVertexBuffer( mVertexBufferHandle, TAC_STACK_FRAME );
      mVertexBufferHandle = Render::CreateVertexBuffer( mDefaultVertex2Ds.size() * sizeof( UI2DVertex ),
                                                        nullptr,
                                                        sizeof( UI2DVertex ),
                                                        Render::Access::Dynamic,
                                                        TAC_STACK_FRAME );
      mVertexCapacity = vertexCount;

      //Render::SetBreakpointWhenThisFrameIsRendered();
    }

    if( !mIndexBufferHandle.IsValid() || mIndexCapacity < indexCount )
    {
      if( mIndexBufferHandle.IsValid() )
        Render::DestroyIndexBuffer( mIndexBufferHandle, TAC_STACK_FRAME );
      Render::Format format;
      format.mElementCount = 1;
      format.mPerElementByteCount = sizeof( UI2DIndex );
      format.mPerElementDataType = Render::GraphicsType::uint;
      mIndexBufferHandle = Render::CreateIndexBuffer( indexCount * sizeof( UI2DIndex ),
                                                      nullptr,
                                                      Render::Access::Dynamic,
                                                      format,
                                                      TAC_STACK_FRAME );
      TAC_HANDLE_ERROR( errors );
      mIndexCapacity = indexCount;
    }


    Render::UpdateVertexBuffer( mVertexBufferHandle,
                                mDefaultVertex2Ds.data(),
                                mDefaultVertex2Ds.size() * sizeof( UI2DVertex ),
                                TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::UpdateIndexBuffer( mIndexBufferHandle,
                               mDefaultIndex2Ds.data(),
                               mDefaultIndex2Ds.size() * sizeof( UI2DIndex ),
                               TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }

  void UI2DDrawData::DrawToTexture( const Render::ViewHandle viewHandle,
                                    int w,
                                    int h,
                                    Errors& errors )
  {
    /*TAC_PROFILE_BLOCK*/;
    //TAC_ASSERT( mStates.empty() );

    if( mDefaultVertex2Ds.size() && mDefaultIndex2Ds.size() )
    {
      UpdateDrawInterface( this, &gDrawInterface, errors );
      Render::VertexBufferHandle& mVertexBufferHandle = gDrawInterface.mVertexBufferHandle;
      Render::IndexBufferHandle& mIndexBufferHandle = gDrawInterface.mIndexBufferHandle;

      OrthographicUIMatrixUnitTest();

      DefaultCBufferPerFrame perFrameData = {};
      perFrameData.mView = m4::Identity();
      perFrameData.mProjection = OrthographicUIMatrix( ( float )w, ( float )h );


      Render::SetBlendState( gUI2DCommonData.mBlendState );
      Render::SetRasterizerState( gUI2DCommonData.mRasterizerState );
      Render::SetSamplerState( gUI2DCommonData.mSamplerState );
      Render::SetDepthState( gUI2DCommonData.mDepthState );
      Render::SetVertexFormat( gUI2DCommonData.mFormat );
      Render::UpdateConstantBuffer( gUI2DCommonData.mPerFrame,
                                    &perFrameData,
                                    sizeof( DefaultCBufferPerFrame ),
                                    TAC_STACK_FRAME );

      for( UI2DDrawCall& uidrawCall : mDrawCall2Ds )
      {

        const Render::TextureHandle texture = uidrawCall.mTexture.IsValid() ?
          uidrawCall.mTexture :
          gUI2DCommonData.m1x1White;
        Render::UpdateConstantBuffer( gUI2DCommonData.mPerObj,
                                      &uidrawCall.mUniformSource,
                                      sizeof( DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );
        Render::SetVertexBuffer( mVertexBufferHandle, uidrawCall.mIVertexStart, uidrawCall.mVertexCount );
        Render::SetIndexBuffer( mIndexBufferHandle, uidrawCall.mIIndexStart, uidrawCall.mIndexCount );
        Render::SetTexture( { texture } );
        Render::SetShader( uidrawCall.mShader );
        Render::Submit( viewHandle, TAC_STACK_FRAME );
      }
    }

    mDrawCall2Ds.resize( 0 );
    mDefaultVertex2Ds.resize( 0 );
    mDefaultIndex2Ds.resize( 0 );
  }

  // cache the results?
  v2 CalculateTextSize( const StringView text,
                        const int fontSize )
  {
    const CodepointView codepoints = UTF8ToCodepoints( text );
    return CalculateTextSize( codepoints, fontSize );
  }

  v2 CalculateTextSize( const CodepointView codepoints,
                        const int fontSize )
  {
    return CalculateTextSize( codepoints.data(),
                              codepoints.size(),
                              fontSize );
  }

  v2 CalculateTextSize( const Codepoint* codepoints,
                        const int codepointCount,
                        const int fontSize )
  {
    float lineWidthUISpaceMax = 0;
    float lineWidthUISpace = 0;
    float xUISpace = 0;


    Language defaultLanguage = Language::English;
    FontFile* fontFile = gFontStuff.mDefaultFonts[ defaultLanguage ];

    int lineCount = 1;

    auto AccountForLine = [&]()
    {
      lineWidthUISpaceMax = Max( lineWidthUISpaceMax, lineWidthUISpace );

      // if the string ends with a space ( ' ' )
      // ( bitmap width is 0, advance width is nonzero )
      lineWidthUISpaceMax = Max( lineWidthUISpaceMax, xUISpace );
    };

    for( int iCodepoint = 0; iCodepoint < codepointCount; ++iCodepoint )
    {
      Codepoint codepoint = codepoints[ iCodepoint ];
      if( !codepoint )
        continue;
      if( IsAsciiCharacter( codepoint ) )
      {
        if( codepoint == '\n' )
        {
          AccountForLine();
          lineWidthUISpace = 0;
          xUISpace = 0;
          lineCount++;
        }
        if( codepoint == '\r' )
          continue;
      }

      FontAtlasCell* fontAtlasCell = nullptr;
      gFontStuff.GetCharacter( defaultLanguage, codepoint, &fontAtlasCell );
      if( !fontAtlasCell )
        continue;

      lineWidthUISpace = xUISpace + fontAtlasCell->mUISpaceLeftSideBearing + fontAtlasCell->mBitmapWidth;
      xUISpace += fontAtlasCell->mUISpaceAdvanceWidth;
    }

    AccountForLine();

    v2 textSize;
    textSize.x = lineWidthUISpaceMax;
    textSize.y = lineCount * ( fontFile->mUISpaceAscent - fontFile->mUISpaceDescent ) +
      ( lineCount - 1 ) * fontFile->mUISpaceLinegap;
    textSize *= ( float )fontSize / ( float )FontCellWidth;

    return textSize;
  }

  void UI2DDrawData::AddBox( v2 mini,
                             v2 maxi,
                             v4 color,
                             Render::TextureHandle texture,
                             const ImGuiRect* clipRect )
  {
    if( clipRect )
    {
      mini = { Max( mini.x, clipRect->mMini.x ),
               Max( mini.y, clipRect->mMini.y ) };
      maxi = { Min( maxi.x, clipRect->mMaxi.x ),
               Min( maxi.y, clipRect->mMaxi.y ) };
    }

    if( mini.x >= maxi.x || mini.y >= maxi.y )
      return;

    int iVert = mDefaultVertex2Ds.size();
    int iIndex = mDefaultIndex2Ds.size();
    mDefaultIndex2Ds.push_back( ( UI2DIndex )iVert + 0 );
    mDefaultIndex2Ds.push_back( ( UI2DIndex )iVert + 1 );
    mDefaultIndex2Ds.push_back( ( UI2DIndex )iVert + 2 );
    mDefaultIndex2Ds.push_back( ( UI2DIndex )iVert + 0 );
    mDefaultIndex2Ds.push_back( ( UI2DIndex )iVert + 2 );
    mDefaultIndex2Ds.push_back( ( UI2DIndex )iVert + 3 );

    mDefaultVertex2Ds.resize( iVert + 4 );
    UI2DVertex* defaultVertex2D = &mDefaultVertex2Ds[ iVert ];

    defaultVertex2D->mPosition = { mini.x, mini.y };
    defaultVertex2D->mGLTexCoord = { 0, 1 };
    defaultVertex2D++;
    defaultVertex2D->mPosition = { mini.x, maxi.y };
    defaultVertex2D->mGLTexCoord = { 0, 0 };
    defaultVertex2D++;
    defaultVertex2D->mPosition = { maxi.x, maxi.y };
    defaultVertex2D->mGLTexCoord = { 1, 0 };
    defaultVertex2D++;
    defaultVertex2D->mPosition = { maxi.x, mini.y };
    defaultVertex2D->mGLTexCoord = { 1, 1 };

    DefaultCBufferPerObject perObjectData = {};
    perObjectData.World = m4::Identity();
    perObjectData.Color = color;

    UI2DDrawCall drawCall;
    drawCall.mIVertexStart = iVert;
    drawCall.mVertexCount = 4;
    drawCall.mIIndexStart = iIndex;
    drawCall.mIndexCount = 6;
    drawCall.mTexture = texture;
    drawCall.mShader = gUI2DCommonData.mShader;
    drawCall.mUniformSource = perObjectData;

    mDrawCall2Ds.push_back( drawCall );
  }

  void UI2DDrawData::AddLine( v2 p0,
                              v2 p1,
                              float radius,
                              v4 color )
  {
    v2 dp = p1 - p0;
    float quadrance = dp.Quadrance();
    if( dp.Quadrance() < 0.01f )
      return;
    float length = std::sqrt( quadrance );
    v2 dphat = dp / length;
    v2 dphatccw = {
      -dphat.y,
      dphat.x,
    };

    int iVert = mDefaultVertex2Ds.size();
    int iIndex = mDefaultIndex2Ds.size();
    mDefaultVertex2Ds.resize( iVert + 4 );
    mDefaultVertex2Ds[ iVert + 0 ].mPosition = p0 + dphatccw * radius;
    mDefaultVertex2Ds[ iVert + 0 ].mGLTexCoord = {};
    mDefaultVertex2Ds[ iVert + 1 ].mPosition = p0 - dphatccw * radius;
    mDefaultVertex2Ds[ iVert + 1 ].mGLTexCoord = {};
    mDefaultVertex2Ds[ iVert + 2 ].mPosition = p1 + dphatccw * radius;
    mDefaultVertex2Ds[ iVert + 2 ].mGLTexCoord = {};
    mDefaultVertex2Ds[ iVert + 3 ].mPosition = p1 - dphatccw * radius;
    mDefaultVertex2Ds[ iVert + 3 ].mGLTexCoord = {};
    mDefaultIndex2Ds.resize( iIndex + 6 );
    mDefaultIndex2Ds[ iIndex + 0 ] = ( UI2DIndex )iVert + 0;
    mDefaultIndex2Ds[ iIndex + 1 ] = ( UI2DIndex )iVert + 1;
    mDefaultIndex2Ds[ iIndex + 2 ] = ( UI2DIndex )iVert + 2;
    mDefaultIndex2Ds[ iIndex + 3 ] = ( UI2DIndex )iVert + 1;
    mDefaultIndex2Ds[ iIndex + 4 ] = ( UI2DIndex )iVert + 3;
    mDefaultIndex2Ds[ iIndex + 5 ] = ( UI2DIndex )iVert + 2;

    DefaultCBufferPerObject perObjectData = {};
    perObjectData.World = m4::Identity();
    perObjectData.Color = color;

    UI2DDrawCall drawCall;
    drawCall.mIVertexStart = iVert;
    drawCall.mVertexCount = 4;
    drawCall.mIIndexStart = iIndex;
    drawCall.mIndexCount = 6;
    drawCall.mTexture;
    drawCall.mShader = gUI2DCommonData.mShader;
    drawCall.mUniformSource = perObjectData;

    mDrawCall2Ds.push_back( drawCall );
  }

  void UI2DDrawData::AddText( const v2 textPos,
                              const int fontSize,
                              StringView utf8,
                              const v4 color,
                              const ImGuiRect* clipRect )
  {
    if( utf8.empty() )
      return;

    CodepointView codepoints = UTF8ToCodepoints( utf8 );

    Language defaultLanguage = Language::English;
    FontFile* fontFile = gFontStuff.mDefaultFonts[ defaultLanguage ];

    float scaleUIToPx = ( float )fontSize / ( float )FontCellWidth;
    float scaleFontToPx = fontFile->mScale * scaleUIToPx;

    float ascentPx = fontFile->mAscent * scaleFontToPx;
    float descentPx = fontFile->mDescent * scaleFontToPx;
    float linegapPx = fontFile->mLinegap * scaleFontToPx;
    float xPxCursor = textPos.x;
    float yPxBaseline = textPos.y + ascentPx;

    int indexStart = mDefaultIndex2Ds.size();
    int indexCount = 0;
    int vertexStart = mDefaultVertex2Ds.size();
    int vertexCount = 0;

    for( Codepoint codepoint : codepoints )
    {
      if( !codepoint )
        continue;

      if( IsAsciiCharacter( codepoint ) )
      {
        if( codepoint == '\n' )
        {
          xPxCursor = textPos.x;
          yPxBaseline += descentPx + linegapPx + ascentPx;
        }
        if( codepoint == '\r' )
          continue;
      }

      FontAtlasCell* fontAtlasCell = nullptr;
      gFontStuff.GetCharacter( defaultLanguage, codepoint, &fontAtlasCell );
      // ^ ignore errors...

      if( !fontAtlasCell )
        continue;

      TAC_ON_DESTRUCT(
        // Even if we don't draw a glyph, we still need to advance the cursor,
        // For example if our current codepoint represents whitespace
        xPxCursor += fontAtlasCell->mAdvanceWidth * scaleFontToPx;
      );

      auto startingIndex = ( UI2DIndex )mDefaultVertex2Ds.size();
      mDefaultIndex2Ds.push_back( startingIndex + 0 );
      mDefaultIndex2Ds.push_back( startingIndex + 1 );
      mDefaultIndex2Ds.push_back( startingIndex + 2 );
      mDefaultIndex2Ds.push_back( startingIndex + 0 );
      mDefaultIndex2Ds.push_back( startingIndex + 2 );
      mDefaultIndex2Ds.push_back( startingIndex + 3 );
      indexCount += 6;

      float glyphMinX = xPxCursor + fontAtlasCell->mLeftSideBearing * scaleFontToPx;
      float glyphMaxX = glyphMinX + fontAtlasCell->mBitmapWidth * scaleUIToPx;
      float glyphMaxY = yPxBaseline - fontAtlasCell->mUISpaceVerticalShift * scaleUIToPx;
      float glyphMinY = glyphMaxY - fontAtlasCell->mBitmapHeight * scaleUIToPx;

      // Coordinate system reminder:
      //
      //   +-->x
      //   |
      //   v
      //   y

      if( clipRect )
      {
        // Check if completely clipped prior to clipping the edges to fit the clip rect.
        // This way we don't need to clip both sides of the glyph against the clip rect.
        if( glyphMaxX < clipRect->mMini.x ||
            glyphMinX > clipRect->mMaxi.x ||
            glyphMaxY < clipRect->mMini.y ||
            glyphMinY > clipRect->mMaxi.y )
          continue;
        glyphMinX = Max( glyphMinX, clipRect->mMini.x );
        glyphMaxX = Min( glyphMaxX, clipRect->mMaxi.x );
        glyphMinY = Max( glyphMinY, clipRect->mMini.y );
        glyphMaxY = Min( glyphMaxY, clipRect->mMaxi.y );
      }

      if( glyphMinX == glyphMaxX || glyphMinY == glyphMaxY )
        continue;
      TAC_ASSERT( glyphMaxX > glyphMinX );
      TAC_ASSERT( glyphMaxY > glyphMinY );

      // todo: compute clipped uvs

      mDefaultVertex2Ds.resize( startingIndex + 4 );
      UI2DVertex* defaultVertex2D = &mDefaultVertex2Ds[ startingIndex ];

      defaultVertex2D->mPosition = { glyphMinX, glyphMinY };
      defaultVertex2D->mGLTexCoord = { fontAtlasCell->mMinGLTexCoord.x, fontAtlasCell->mMaxGLTexCoord.y };
      defaultVertex2D++;
      defaultVertex2D->mPosition = { glyphMinX, glyphMaxY };
      defaultVertex2D->mGLTexCoord = { fontAtlasCell->mMinGLTexCoord.x, fontAtlasCell->mMinGLTexCoord.y };
      defaultVertex2D++;
      defaultVertex2D->mPosition = { glyphMaxX, glyphMaxY };
      defaultVertex2D->mGLTexCoord = { fontAtlasCell->mMaxGLTexCoord.x, fontAtlasCell->mMinGLTexCoord.y };
      defaultVertex2D++;
      defaultVertex2D->mPosition = { glyphMaxX, glyphMinY };
      defaultVertex2D->mGLTexCoord = { fontAtlasCell->mMaxGLTexCoord.x, fontAtlasCell->mMaxGLTexCoord.y };
      vertexCount += 4;
    }

    DefaultCBufferPerObject perObjectData = {};
    perObjectData.World = m4::Identity();
    perObjectData.Color = color;

    UI2DDrawCall drawCall;
    drawCall.mIndexCount = indexCount;
    drawCall.mIIndexStart = indexStart;
    drawCall.mVertexCount = vertexCount;
    drawCall.mIVertexStart = vertexStart;
    drawCall.mTexture = gFontStuff.mTextureId;
    drawCall.mShader = gUI2DCommonData.m2DTextShader;
    drawCall.mUniformSource = perObjectData;
    mDrawCall2Ds.push_back( drawCall );
  }

}
