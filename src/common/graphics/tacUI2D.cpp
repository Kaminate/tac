#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/containers/tacArray.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/math/tacMath.h"

namespace Tac
{

  //static VertexBufferData GetVertexBufferData( const StackFrame& frame, int vertexCount )
  //{
  //  VertexBufferData vertexBufferData = {};
  //  vertexBufferData.mAccess = Access::Dynamic;
  //  vertexBufferData.mName = "draw data verts";
  //  vertexBufferData.mStrideBytesBetweenVertexes = sizeof( UI2DVertex );
  //  vertexBufferData.mNumVertexes = vertexCount;
  //  vertexBufferData.mFrame = frame;
  //  return vertexBufferData;
  //}

  //static IndexBufferData GetIndexBufferData( const StackFrame& frame, int indexCount )
  //{
  //  IndexBufferData indexBufferData;
  //  indexBufferData.mAccess = Access::Dynamic;
  //  indexBufferData.mName = "draw data indexes";
  //  indexBufferData.mFormat.mPerElementDataType = GraphicsType::uint;
  //  indexBufferData.mFormat.mElementCount = 1;
  //  indexBufferData.mFormat.mPerElementByteCount = sizeof( UI2DIndex );
  //  indexBufferData.mFrame = frame;
  //  indexBufferData.mIndexCount = indexCount;
  //  return indexBufferData;
  //}

  UI2DCommonData* UI2DCommonData::Instance = nullptr;
  UI2DCommonData::UI2DCommonData()
  {
    Instance = this;
  }
  UI2DCommonData::~UI2DCommonData()
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
  void UI2DCommonData::Init( Errors& errors )
  {
    uint8_t data[] = { 255, 255, 255, 255 };
    Render::TexSpec textureData = {};
    textureData.mImage.mWidth = 1;
    textureData.mImage.mHeight = 1;
    textureData.mPitch = 1;
    textureData.mImage.mFormat.mElementCount = 4;
    textureData.mImage.mFormat.mPerElementByteCount = 1;
    textureData.mImage.mFormat.mPerElementDataType = GraphicsType::unorm;
    textureData.mImageBytes = data;
    textureData.mBinding = Binding::ShaderResource;
    m1x1White = Render::CreateTexture( "1x1 white", textureData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mPerFrame = Render::CreateConstantBuffer( "tac ui 2d per frame",
                                              sizeof( DefaultCBufferPerFrame ),
                                              0,

                                              TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mPerObj = Render::CreateConstantBuffer( "tac ui 2d per obj",
                                            sizeof( DefaultCBufferPerObject ),
                                            1,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mShader = Render::CreateShader( "tac 2d ui shader",
                                    Render::ShaderSource::FromPath( "2D" ),
                                    Render::ConstantBuffers( mPerFrame, mPerObj ),
                                    TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    m2DTextShader = Render::CreateShader( "tac 2d ui text shader",
                                          Render::ShaderSource::FromPath( "2Dtext" ),
                                          Render::ConstantBuffers( mPerFrame, mPerObj ),
                                          TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    VertexDeclaration posData;
    posData.mAlignedByteOffset = TAC_OFFSET_OF( UI2DVertex, mPosition );
    posData.mAttribute = Attribute::Position;
    posData.mTextureFormat = formatv2;
    VertexDeclaration uvData;
    uvData.mAlignedByteOffset = TAC_OFFSET_OF( UI2DVertex, mGLTexCoord );
    uvData.mAttribute = Attribute::Texcoord;
    uvData.mTextureFormat = formatv2;
    Render::VertexDeclarations vertexDeclarations;
    vertexDeclarations.AddVertexDeclaration( posData );
    vertexDeclarations.AddVertexDeclaration( uvData );
    mFormat = Render::CreateVertexFormat( "tac 2d ui vertex format",
                                          vertexDeclarations,
                                          mShader,
                                          TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::BlendState blendStateData;
    blendStateData.srcRGB = BlendConstants::One;
    blendStateData.dstRGB = BlendConstants::OneMinusSrcA;
    blendStateData.blendRGB = BlendMode::Add;
    blendStateData.srcA = BlendConstants::One;
    blendStateData.dstA = BlendConstants::OneMinusSrcA;
    blendStateData.blendA = BlendMode::Add;
    mBlendState = Render::CreateBlendState( "tac 2d ui alpha blend state",
                                            blendStateData,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::DepthState depthStateData;
    depthStateData.mDepthTest = false;
    depthStateData.mDepthWrite = false;
    depthStateData.mDepthFunc = DepthFunc::Less;
    mDepthState = Render::CreateDepthState( "tac 2d ui no depth read/write",
                                            depthStateData,
                                            TAC_STACK_FRAME );

    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = CullMode::None;
    rasterizerStateData.mFillMode = FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( "tac 2d ui no cull",
                                                      rasterizerStateData,
                                                      TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Filter::Linear;
    mSamplerState = Render::CreateSamplerState( "tac 2d ui tex sampler",
                                                samplerStateData,
                                                TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }

  UI2DDrawData::UI2DDrawData()
  {


  }
  UI2DDrawData::~UI2DDrawData()
  {
    if( mVertexBufferHandle.IsValid() )
      Render::DestroyVertexBuffer( mVertexBufferHandle, TAC_STACK_FRAME );
    if( mIndexBufferHandle.IsValid() )
      Render::DestroyIndexBuffer( mIndexBufferHandle, TAC_STACK_FRAME );
  }
  void UI2DDrawData::DrawToTexture( int viewWidth, int viewHeight, Render::ViewId viewId, Errors& errors )
  {
    TAC_ASSERT( viewWidth );
    TAC_ASSERT( viewHeight );

    /*TAC_PROFILE_BLOCK*/;
    TAC_ASSERT( mStates.empty() );

    int vertexCount = mDefaultVertex2Ds.size();
    int indexCount = mDefaultIndex2Ds.size();
    if( vertexCount && indexCount )
    {
      if( !mVertexBufferHandle.IsValid() || mVertexCapacity < vertexCount )
      {
        if( mVertexBufferHandle.IsValid() )
          Render::DestroyVertexBuffer( mVertexBufferHandle, TAC_STACK_FRAME );
        mVertexBufferHandle = Render::CreateVertexBuffer( "draw data verts",
                                                          mDefaultVertex2Ds.size() * sizeof( UI2DVertex ),
                                                          nullptr,
                                                          sizeof( UI2DVertex ),
                                                          Access::Dynamic,
                                                          TAC_STACK_FRAME );
        mVertexCapacity = vertexCount;
      }

      if( !mIndexBufferHandle.IsValid() || mIndexCapacity < indexCount )
      {
        if( mIndexBufferHandle.IsValid() )
          Render::DestroyIndexBuffer( mIndexBufferHandle, TAC_STACK_FRAME );
        Format format;
        format.mElementCount = 1;
        format.mPerElementByteCount = sizeof( UI2DIndex );
        format.mPerElementDataType = GraphicsType::uint;
        mIndexBufferHandle = Render::CreateIndexBuffer( "draw data indexes",
                                                        indexCount * sizeof( UI2DIndex ),
                                                        nullptr,
                                                        Access::Dynamic,
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
      //mIndexes->Overwrite( mDefaultIndex2Ds.data(), indexCount * sizeof( UI2DIndex ), errors );
      TAC_HANDLE_ERROR( errors );

      // mRenderView->mViewportRect.mViewportPixelWidthIncreasingRight?
      float sx = 2.0f / viewWidth;
      float sy = 2.0f / viewHeight;
      auto projectionPieces = MakeArray< m4 >(
        // orient to bottom left
        m4( 1, 0, 0, 0,
            0, -1, 0, ( float )viewHeight,
            0, 0, 1, 0,
            0, 0, 0, 1 ),
        // convert to ndc
        m4( sx, 0, 0, -1,
            0, sy, 0, -1,
            0, 0, 1, 0,
            0, 0, 0, 1 ) );
      static v4 testVectorOrig( 130, 160, 0, 1 );
      v4 testVector = testVectorOrig;
      static bool testVectorMultiply = true;
      static bool testVectorPrint = false;
      auto PrintTestVector = [ & ]()
      {
        if( testVectorPrint )
          std::cout
          << va(
            "Test vector: %.2f, %.2f, %.2f\n",
            testVector.x,
            testVector.y,
            testVector.z )
          << std::endl;
      };
      PrintTestVector();
      m4 projection = m4::Identity();
      for( m4 projectionPiece : projectionPieces )
      {
        projection = projectionPiece * projection;
        testVector = projectionPiece * testVector;
        PrintTestVector();
      }

      DefaultCBufferPerFrame perFrameData = {};
      perFrameData.mView = m4::Identity();
      perFrameData.mProjection = projection;


      DrawCall2 perFrame = {};
      //perFrame.mRenderView = mRenderView;
      //perFrame.mBlendState = UI2DCommonData::Instance->mBlendState;
      //perFrame.mRasterizerState = UI2DCommonData::Instance->mRasterizerState;
      //perFrame.mSamplerState = UI2DCommonData::Instance->mSamplerState;
      //perFrame.mDepthState = UI2DCommonData::Instance->mDepthState;
      //perFrame.mVertexFormat = UI2DCommonData::Instance->mFormat;
      //perFrame.mUniformDst = UI2DCommonData::Instance->mPerFrame;
      //perFrame.mUniformSrcc = TemporaryMemoryFromT( perFrameData );
      //perFrame.mFrame = TAC_STACK_FRAME;
      //Render::AddDrawCall( perFrame );

      Render::SetBlendState( UI2DCommonData::Instance->mBlendState );
      Render::SetRasterizerState( UI2DCommonData::Instance->mRasterizerState );
      Render::SetSamplerState( UI2DCommonData::Instance->mSamplerState );
      Render::SetDepthState( UI2DCommonData::Instance->mDepthState );
      Render::SetVertexFormat( UI2DCommonData::Instance->mFormat );

      Render::UpdateConstantBuffer( UI2DCommonData::Instance->mPerFrame,
                                    &perFrameData,
                                    sizeof( DefaultCBufferPerFrame ),
                                    TAC_STACK_FRAME );

      for( UI2DDrawCall& uidrawCall : mDrawCall2Ds )
      {

        Render::TextureHandle texture = uidrawCall.mTexture.IsValid() ?
          uidrawCall.mTexture :
          UI2DCommonData::Instance->m1x1White;

        Render::UpdateConstantBuffer( UI2DCommonData::Instance->mPerObj,
                                      &uidrawCall.mUniformSource,
                                      sizeof( DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );

        //DrawCall2 drawCall2 = {};
        //drawCall2.mUniformDst = UI2DCommonData::Instance->mPerObj;
        //drawCall2.mVertexBuffer = mVerts;
        //drawCall2.mRenderView = mRenderView;
        //drawCall2.mBlendState = UI2DCommonData::Instance->mBlendState;
        //drawCall2.mRasterizerState = UI2DCommonData::Instance->mRasterizerState;
        //drawCall2.mSamplerState = UI2DCommonData::Instance->mSamplerState;
        //drawCall2.mDepthState = UI2DCommonData::Instance->mDepthState;
        //drawCall2.mVertexFormat = UI2DCommonData::Instance->mFormat;
        Render::SetVertexBuffer( mVertexBufferHandle, uidrawCall.mIVertexStart, uidrawCall.mVertexCount );
        Render::SetIndexBuffer( mIndexBufferHandle, uidrawCall.mIIndexStart, uidrawCall.mIndexCount );
        Render::SetTexture( texture );
        //drawCall2.mIndexBuffer = mIndexes;
        //drawCall2.mStartIndex = uidrawCall.mIIndexStart;
        //drawCall2.mIndexCount = uidrawCall.mIndexCount;


        //drawCall2.mTextureHandles = { texture };
        //drawCall2.mShader = uidrawCall.mShader;
        //drawCall2.mUniformSrcc = TemporaryMemoryFromT( uidrawCall.mUniformSource );
        //drawCall2.mFrame = TAC_STACK_FRAME;
        Render::SetShader( uidrawCall.mShader );
        //Render::AddDrawCall( drawCall2 );
        Render::Submit( viewId );
      }
    }

    mDrawCall2Ds.clear();
    mDefaultVertex2Ds.clear();
    mDefaultIndex2Ds.clear();

    //Renderer::Instance->DebugBegin( "2d" );
    //Renderer::Instance->RenderFlush();
    //Renderer::Instance->DebugEnd();
  }
  UI2DState* UI2DDrawData::PushState()
  {
    UI2DState state;
    state.mUI2DDrawData = this;
    if( !mStates.empty() )
      state = mStates.back();
    mStates.push_back( state );
    return &mStates.back();
  }
  void UI2DDrawData::PopState()
  {
    TAC_ASSERT( mStates.size() );
    mStates.pop_back();
  }

  void UI2DState::Draw2DBox(
    float width,
    float height,
    v4 color,
    Render::TextureHandle texture )
  {
    if( width <= 0 || height <= 0 )
      return;
    auto oldVertexCount = ( int )mUI2DDrawData->mDefaultVertex2Ds.size();
    auto oldIndexCount = ( int )mUI2DDrawData->mDefaultIndex2Ds.size();
    auto offsets = MakeArray<v2>(
      v2( 0, 0 ),
      v2( 0, height ),
      v2( width, height ),
      v2( width, 0 ) );
    auto uvs = MakeArray<v2>(
      v2{ 0, 1 },
      v2{ 0, 0 },
      v2{ 1, 0 },
      v2{ 1, 1 } );
    const int vertexCount = 4;
    TAC_ASSERT( offsets.size() == vertexCount );
    TAC_ASSERT( uvs.size() == vertexCount );

    for( v2& offset : offsets )
      offset = ( mTransform * v3( offset, 1.0f ) ).xy();

    for( int iVert = 0; iVert < vertexCount; ++iVert )
    {
      UI2DVertex defaultVertex2D;
      defaultVertex2D.mGLTexCoord = uvs[ iVert ];
      defaultVertex2D.mPosition = offsets[ iVert ];
      mUI2DDrawData->mDefaultVertex2Ds.push_back( defaultVertex2D );
    }

    UI2DIndex indexes[] = { 0, 1, 2, 0, 2, 3 };
    for( int offset : indexes )
      mUI2DDrawData->mDefaultIndex2Ds.push_back( oldVertexCount + offset );

    DefaultCBufferPerObject perObjectData = {};
    perObjectData.World = m4::Identity();
    perObjectData.Color = color;

    UI2DDrawCall drawCall;
    drawCall.mIVertexStart = oldVertexCount;
    drawCall.mVertexCount = vertexCount;
    drawCall.mIIndexStart = oldIndexCount;
    drawCall.mIndexCount = 6;
    drawCall.mTexture = texture;
    drawCall.mShader = UI2DCommonData::Instance->mShader;
    drawCall.mUniformSource = perObjectData;

    mUI2DDrawData->mDrawCall2Ds.push_back( drawCall );
  }

  void UI2DState::Draw2DText( Language defaultLanguage,
                              int fontSize,
                              StringView text,
                              float* heightBetweenBaselines,
                              v4 color,
                              Errors& errors )
  {
    const m3& transform = mTransform;

    Vector< Codepoint > codepoints;
    UTF8Converter::Convert( text, codepoints, errors );
    TAC_HANDLE_ERROR( errors );
    auto oldIndexCount = ( int )mUI2DDrawData->mDefaultIndex2Ds.size();
    auto oldVertexCount = ( int )mUI2DDrawData->mDefaultVertex2Ds.size();
    float scale = ( float )fontSize / FontCellWidth;
    float lineHeight = scale * FontCellWidth;


    int lineCount = 1;

    // ???
    v2 mPenPos2D = {};

    // The pen pos starts at the top of the line above us,
    // so reserve some space for the first line.
    float runningY = mPenPos2D.y + lineHeight;
    float runningX = mPenPos2D.x;

    float minY = runningY;
    float maxY = runningY;
    float minX = runningX;
    float maxX = runningX;

    for( Codepoint codepoint : codepoints )
    {
      if( !codepoint )
        continue;

      if( IsAsciiCharacter( codepoint ) )
      {
        if( codepoint == '\n' )
        {
          runningX = mPenPos2D.x;
          lineCount++;
          runningY += lineHeight;
        }
        if( codepoint == '\r' )
          continue;
      }


      FontAtlasCell* fontAtlasCell;
      FontStuff::Instance->GetCharacter( defaultLanguage, codepoint, &fontAtlasCell, errors );
      TAC_HANDLE_ERROR( errors );
      if( !fontAtlasCell )
        continue;

      const int quad_vertex_count = 4;
      auto startingIndex = ( int )mUI2DDrawData->mDefaultVertex2Ds.size();
      auto indexes = MakeArray< int >( 0, 1, 2, 0, 2, 3 );
      for( int iVert : indexes )
      {
        mUI2DDrawData->mDefaultIndex2Ds.push_back( startingIndex + iVert );
      }
      const v2 glTexCoords[ quad_vertex_count ] = {
        v2( 0, 0 ),
        v2( 1, 0 ),
        v2( 1, 1 ),
        v2( 0, 1 ) };

      float fontAtlasCellX = runningX + fontAtlasCell->mUISpaceLeftSideBearing * scale;
      float fontAtlasCellY = runningY - fontAtlasCell->mUISpaceVerticalShift * scale;
      float scaledBitmapWidth = fontAtlasCell->mBitmapWidth * scale;
      float scaledBitmapHeight = fontAtlasCell->mBitmapHeight * scale;

      minY = Min( minY, fontAtlasCellY );
      maxY = Max( maxY, fontAtlasCellY + scaledBitmapHeight );
      maxX = Max( maxX, fontAtlasCellX + scaledBitmapWidth );

      for( const v2& glTexCoord : glTexCoords )
      {
        v3 position3 = transform * v3(
          fontAtlasCellX + glTexCoord.x * scaledBitmapWidth,
          fontAtlasCellY - glTexCoord.y * scaledBitmapHeight,
          1 );
        v2 position2 = position3.xy();

        UI2DVertex vert2D;
        vert2D.mPosition = position2;
        vert2D.mGLTexCoord.x = Lerp(
          fontAtlasCell->mMinGLTexCoord.x,
          fontAtlasCell->mMaxGLTexCoord.x,
          glTexCoord.x );
        vert2D.mGLTexCoord.y = Lerp(
          fontAtlasCell->mMinGLTexCoord.y,
          fontAtlasCell->mMaxGLTexCoord.y,
          glTexCoord.y );
        mUI2DDrawData->mDefaultVertex2Ds.push_back( vert2D );
      }

      runningX += fontAtlasCell->mUISpaceAdvanceWidth * scale;
    }

    DefaultCBufferPerObject perObjectData = {};
    perObjectData.World = m4::Identity();
    perObjectData.Color = color;

    UI2DDrawCall drawCall;
    drawCall.mIndexCount = ( int )mUI2DDrawData->mDefaultIndex2Ds.size() - oldIndexCount;
    drawCall.mIIndexStart = oldIndexCount;
    drawCall.mVertexCount = ( int )mUI2DDrawData->mDefaultVertex2Ds.size() - oldVertexCount;
    drawCall.mIVertexStart = oldVertexCount;
    drawCall.mTexture = FontStuff::Instance->mTextureId;
    drawCall.mShader = UI2DCommonData::Instance->m2DTextShader;
    drawCall.mUniformSource = perObjectData;
    //if( !drawCall.mScissorDebugging )
    //{
    //  drawCall.mScissorTest = true;
    //  drawCall.mScissorRectMinUISpace.x = minX;
    //  drawCall.mScissorRectMinUISpace.y = minY;
    //  drawCall.mScissorRectMaxUISpace.x = maxX;
    //  drawCall.mScissorRectMaxUISpace.y = maxY;
    //}
    mUI2DDrawData->mDrawCall2Ds.push_back( drawCall );

    if( heightBetweenBaselines )
      *heightBetweenBaselines = ( float )( lineCount * lineHeight );
  }

  // cache the results?
  v2 UI2DDrawData::CalculateTextSize( StringView text, int fontSize )
  {
    Vector< Codepoint > codepoints;

    // ignored
    Errors errors;

    UTF8Converter::Convert( text, codepoints, errors );
    return CalculateTextSize( codepoints, fontSize );
  }

  v2 UI2DDrawData::CalculateTextSize( const Vector< Codepoint >& codepoints, int fontSize )
  {
    return CalculateTextSize( codepoints.data(), ( int )codepoints.size(), fontSize );
  }
  v2 UI2DDrawData::CalculateTextSize( const Codepoint* codepoints, int codepointCount, int fontSize )
  {
    float lineWidthUISpaceMax = 0;
    float lineWidthUISpace = 0;
    float xUISpace = 0;


    Language defaultLanguage = Language::English;
    FontFile* fontFile = FontStuff::Instance->mDefaultFonts[ defaultLanguage ];

    int lineCount = 1;

    auto AccountForLine = [ & ]()
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

      // ignored...
      Errors errors;

      FontStuff::Instance->GetCharacter( defaultLanguage, codepoint, &fontAtlasCell, errors );

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

  void UI2DDrawData::AddBox( v2 mini, v2 maxi, v4 color, Render::TextureHandle texture, const ImGuiRect* clipRect )
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
    mDefaultIndex2Ds.push_back( iVert + 0 );
    mDefaultIndex2Ds.push_back( iVert + 1 );
    mDefaultIndex2Ds.push_back( iVert + 2 );
    mDefaultIndex2Ds.push_back( iVert + 0 );
    mDefaultIndex2Ds.push_back( iVert + 2 );
    mDefaultIndex2Ds.push_back( iVert + 3 );

    mDefaultVertex2Ds.resize( iVert + 4 );
    UI2DVertex* defaultVertex2D = &mDefaultVertex2Ds[ iVert ];

    defaultVertex2D->mPosition = { mini.x, mini.y };
    defaultVertex2D->mGLTexCoord = {};
    defaultVertex2D++;
    defaultVertex2D->mPosition = { mini.x, maxi.y };
    defaultVertex2D->mGLTexCoord = {};
    defaultVertex2D++;
    defaultVertex2D->mPosition = { maxi.x, maxi.y };
    defaultVertex2D->mGLTexCoord = {};
    defaultVertex2D++;
    defaultVertex2D->mPosition = { maxi.x, mini.y };
    defaultVertex2D->mGLTexCoord = {};

    DefaultCBufferPerObject perObjectData = {};
    perObjectData.World = m4::Identity();
    perObjectData.Color = color;

    UI2DDrawCall drawCall;
    drawCall.mIVertexStart = iVert;
    drawCall.mVertexCount = 4;
    drawCall.mIIndexStart = iIndex;
    drawCall.mIndexCount = 6;
    drawCall.mTexture = texture;
    drawCall.mShader = UI2DCommonData::Instance->mShader;
    drawCall.mUniformSource = perObjectData;

    mDrawCall2Ds.push_back( drawCall );
  }

  void UI2DDrawData::AddLine( v2 p0, v2 p1, float radius, v4 color )
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
    mDefaultIndex2Ds[ iIndex + 0 ] = iVert + 0;
    mDefaultIndex2Ds[ iIndex + 1 ] = iVert + 1;
    mDefaultIndex2Ds[ iIndex + 2 ] = iVert + 2;
    mDefaultIndex2Ds[ iIndex + 3 ] = iVert + 1;
    mDefaultIndex2Ds[ iIndex + 4 ] = iVert + 3;
    mDefaultIndex2Ds[ iIndex + 5 ] = iVert + 2;

    DefaultCBufferPerObject perObjectData = {};
    perObjectData.World = m4::Identity();
    perObjectData.Color = color;

    UI2DDrawCall drawCall;
    drawCall.mIVertexStart = iVert;
    drawCall.mVertexCount = 4;
    drawCall.mIIndexStart = iIndex;
    drawCall.mIndexCount = 6;
    drawCall.mTexture;
    drawCall.mShader = UI2DCommonData::Instance->mShader;
    drawCall.mUniformSource = perObjectData;

    mDrawCall2Ds.push_back( drawCall );
  }

  //void UI2DDrawData::AddPolyFill( const Vector< v2 >& points, v4 color )
  //{
  //}
  //void UI2DDrawData::AddSquiggle( const Vector< v2 >& inputPoints, float width, v4 color )
  //{
  //  int iVert = mDefaultVertex2Ds.size();
  //  int iIndex = mDefaultIndex2Ds.size();
  //
  //  Vector< v2 > outputPoints;
  //  outputPoints.resize( 2 * inputPoints.size() - 2 );
  //
  //  mDefaultVertex2Ds.resize( iVert + outputPoints.size() );
  //
  //  DefaultVertex2D* defaultVertex2D = &mDefaultVertex2Ds[ iVert ];
  //
  //
  //  Vector< v2 > normals;
  //}

  void UI2DDrawData::AddText(
    const v2 textPos,
    const int fontSize,
    StringView utf8,
    const v4 color,
    const ImGuiRect* clipRect )
  {
    if( utf8.empty() )
      return;

    // ignored
    Errors errors;

    Vector< Codepoint > codepoints;
    UTF8Converter::Convert( utf8, codepoints, errors );

    Language defaultLanguage = Language::English;
    FontFile* fontFile = FontStuff::Instance->mDefaultFonts[ defaultLanguage ];

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
      FontStuff::Instance->GetCharacter( defaultLanguage, codepoint, &fontAtlasCell, errors );
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

      if( utf8 == "Entity 1" && ( char )codepoint == '1' )
      {
        static int i;
        ++i;
      }

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
    drawCall.mTexture = FontStuff::Instance->mTextureId;
    drawCall.mShader = UI2DCommonData::Instance->m2DTextShader;
    drawCall.mUniformSource = perObjectData;
    mDrawCall2Ds.push_back( drawCall );
  }

  void UI2DState::Translate( v2 pos )
  {
    Translate( pos.x, pos.y );
  }
  void UI2DState::Translate( float x, float y )
  {
    mTransform = mTransform * M3Translate( x, y );
  }


  //void UI2DDrawCall::CopyUniform( const void* bytes, int byteCount )
  //{
  //  mUniformSource.resize( byteCount );
  //  MemCpy( mUniformSource.data(), bytes, byteCount );
  //}
}
