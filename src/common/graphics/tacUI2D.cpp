#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacRenderer.h"
#include "common/graphics/imgui/tacImGui.h"
#include "common/containers/tacArray.h"
#include "common/profile/tacProfile.h"
#include "common/math/tacMath.h"

static TacVertexBufferData GetVertexBufferData( const TacStackFrame& stackFrame, int vertexCount )
{
  TacVertexBufferData vertexBufferData = {};
  vertexBufferData.mAccess = TacAccess::Dynamic;
  vertexBufferData.mName = "draw data verts";
  vertexBufferData.mStrideBytesBetweenVertexes = sizeof( TacUI2DVertex );
  vertexBufferData.mNumVertexes = vertexCount;
  vertexBufferData.mStackFrame = stackFrame;
  return vertexBufferData;
}

static TacIndexBufferData GetIndexBufferData( const TacStackFrame& stackFrame, int indexCount )
{
  TacIndexBufferData indexBufferData;
  indexBufferData.mAccess = TacAccess::Dynamic;
  indexBufferData.mName = "draw data indexes";
  indexBufferData.mFormat.mPerElementDataType = TacGraphicsType::uint;
  indexBufferData.mFormat.mElementCount = 1;
  indexBufferData.mFormat.mPerElementByteCount = sizeof( TacUI2DIndex );
  indexBufferData.mStackFrame = stackFrame;
  indexBufferData.mIndexCount = indexCount;
  return indexBufferData;
}

TacUI2DCommonData* TacUI2DCommonData::Instance = nullptr;
TacUI2DCommonData::TacUI2DCommonData()
{
  Instance = this;
}
TacUI2DCommonData::~TacUI2DCommonData()
{
  TacRenderer::Instance->RemoveRendererResource( m1x1White );
  TacRenderer::Instance->RemoveRendererResource( mFormat );
  TacRenderer::Instance->RemoveRendererResource( mShader );
  TacRenderer::Instance->RemoveRendererResource( m2DTextShader );
  TacRenderer::Instance->RemoveRendererResource( mDepthState );
  TacRenderer::Instance->RemoveRendererResource( mBlendState );
  TacRenderer::Instance->RemoveRendererResource( mRasterizerState );
  TacRenderer::Instance->RemoveRendererResource( mSamplerState );
  TacRenderer::Instance->RemoveRendererResource( mPerFrame );
  TacRenderer::Instance->RemoveRendererResource( mPerObj );
}
void TacUI2DCommonData::Init( TacErrors& errors )
{
  uint8_t data[] = { 255, 255, 255, 255 };
  TacTextureData textureData = {};
  textureData.myImage.mWidth = 1;
  textureData.myImage.mHeight = 1;
  textureData.myImage.mPitch = sizeof( data );
  textureData.myImage.mData = data;
  textureData.myImage.mFormat.mElementCount = 4;
  textureData.myImage.mFormat.mPerElementByteCount = 1;
  textureData.myImage.mFormat.mPerElementDataType = TacGraphicsType::unorm;
  textureData.mStackFrame = TAC_STACK_FRAME;
  textureData.mName = "1x1 white";
  textureData.binding = { TacBinding::ShaderResource };
  TacRenderer::Instance->AddTextureResource( &m1x1White, textureData, errors );
  TAC_HANDLE_ERROR( errors );

  TacCBufferData cBufferDataPerFrame = {};
  cBufferDataPerFrame.mName = "tac ui 2d per frame";
  cBufferDataPerFrame.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerFrame.shaderRegister = 0;
  cBufferDataPerFrame.byteCount = sizeof( TacDefaultCBufferPerFrame );
  TacRenderer::Instance->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
  TAC_HANDLE_ERROR( errors );

  TacCBufferData cBufferDataPerObj = {};
  cBufferDataPerObj.mName = "tac ui 2d per obj";
  cBufferDataPerObj.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerObj.shaderRegister = 1;
  cBufferDataPerObj.byteCount = sizeof( TacDefaultCBufferPerObject );
  TacRenderer::Instance->AddConstantBuffer( &mPerObj, cBufferDataPerObj, errors );
  TAC_HANDLE_ERROR( errors );

  TacShaderData shaderData;
  shaderData.mName = "tac 2d ui shader";
  shaderData.mShaderPath = "2D";
  shaderData.mStackFrame = TAC_STACK_FRAME;
  shaderData.mCBuffers = { mPerFrame, mPerObj };
  TacRenderer::Instance->AddShader( &mShader, shaderData, errors );
  TAC_HANDLE_ERROR( errors );

  TacShaderData textShaderData;
  textShaderData.mName = "tac 2d ui text shader";
  textShaderData.mShaderPath = "2Dtext";
  textShaderData.mStackFrame = TAC_STACK_FRAME;
  textShaderData.mCBuffers = { mPerFrame, mPerObj };
  TacRenderer::Instance->AddShader( &m2DTextShader, textShaderData, errors );
  TAC_HANDLE_ERROR( errors );

  TacVertexDeclaration posData;
  posData.mAlignedByteOffset = TacOffsetOf( TacUI2DVertex, mPosition );
  posData.mAttribute = TacAttribute::Position;
  posData.mTextureFormat = formatv2;
  TacVertexDeclaration uvData;
  uvData.mAlignedByteOffset = TacOffsetOf( TacUI2DVertex, mGLTexCoord );
  uvData.mAttribute = TacAttribute::Texcoord;
  uvData.mTextureFormat = formatv2;
  TacVertexFormatData vertexFormatData = {};
  vertexFormatData.mName = "tac 2d ui vertex format";
  vertexFormatData.mStackFrame = TAC_STACK_FRAME;
  vertexFormatData.shader = mShader;
  vertexFormatData.vertexFormatDatas = { posData, uvData };
  TacRenderer::Instance->AddVertexFormat( &mFormat, vertexFormatData, errors );
  TAC_HANDLE_ERROR( errors );

  TacBlendStateData blendStateData;
  blendStateData.srcRGB = TacBlendConstants::One;
  blendStateData.dstRGB = TacBlendConstants::OneMinusSrcA;
  blendStateData.blendRGB = TacBlendMode::Add;
  blendStateData.srcA = TacBlendConstants::One;
  blendStateData.dstA = TacBlendConstants::OneMinusSrcA;
  blendStateData.blendA = TacBlendMode::Add;
  blendStateData.mName = "tac 2d ui alpha blend state";
  blendStateData.mStackFrame = TAC_STACK_FRAME;
  TacRenderer::Instance->AddBlendState( &mBlendState, blendStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacDepthStateData depthStateData;
  depthStateData.depthTest = false;
  depthStateData.depthWrite = false;
  depthStateData.depthFunc = TacDepthFunc::Less;
  depthStateData.mName = "tac 2d ui no depth read/write";
  depthStateData.mStackFrame = TAC_STACK_FRAME;
  TacRenderer::Instance->AddDepthState( &mDepthState, depthStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacRasterizerStateData rasterizerStateData;
  rasterizerStateData.cullMode = TacCullMode::None;
  rasterizerStateData.fillMode = TacFillMode::Solid;
  rasterizerStateData.frontCounterClockwise = true;
  rasterizerStateData.mName = "tac 2d ui no cull";
  rasterizerStateData.mStackFrame = TAC_STACK_FRAME;
  rasterizerStateData.multisample = false;
  rasterizerStateData.scissor = true;
  TacRenderer::Instance->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacSamplerStateData samplerStateData;
  samplerStateData.mName = "tac 2d ui tex sampler";
  samplerStateData.mStackFrame = TAC_STACK_FRAME;
  samplerStateData.filter = TacFilter::Linear;
  TacRenderer::Instance->AddSamplerState( &mSamplerState, samplerStateData, errors );
  TAC_HANDLE_ERROR( errors );
}

  TacUI2DDrawData::TacUI2DDrawData()
  {


  }
TacUI2DDrawData::~TacUI2DDrawData()
{
  if( mVerts )
    TacRenderer::Instance->RemoveRendererResource( mVerts );
  if( mIndexes )
    TacRenderer::Instance->RemoveRendererResource( mIndexes );
}
void TacUI2DDrawData::DrawToTexture( TacErrors& errors )
{
  /*TAC_PROFILE_BLOCK*/;
  TacAssert( mStates.empty() );

  int vertexCount = mDefaultVertex2Ds.size();
  int indexCount = mDefaultIndex2Ds.size();
  if( vertexCount && indexCount )
  {
    if( !mVerts || mVerts->mNumVertexes < vertexCount )
    {
      if( mVerts )
        TacRenderer::Instance->RemoveRendererResource( mVerts );
      TacVertexBufferData vertexBufferData = GetVertexBufferData( TAC_STACK_FRAME, vertexCount );
      TacRenderer::Instance->AddVertexBuffer( &mVerts, vertexBufferData, errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( !mIndexes || mIndexes->mIndexCount < indexCount )
    {
      if( mIndexes )
        TacRenderer::Instance->RemoveRendererResource( mIndexes );
      TacIndexBufferData indexBufferData = GetIndexBufferData( TAC_STACK_FRAME, indexCount );
      TacRenderer::Instance->AddIndexBuffer( &mIndexes, indexBufferData, errors );
      TAC_HANDLE_ERROR( errors );
    }

    mVerts->Overwrite( mDefaultVertex2Ds.data(), vertexCount * sizeof( TacUI2DVertex ), errors );
    TAC_HANDLE_ERROR( errors );

    mIndexes->Overwrite( mDefaultIndex2Ds.data(), indexCount * sizeof( TacUI2DIndex ), errors );
    TAC_HANDLE_ERROR( errors );

    // mRenderView->mViewportRect.mViewportPixelWidthIncreasingRight?
    float sx = 2.0f / mRenderView->mFramebuffer->myImage.mWidth;
    float sy = 2.0f / mRenderView->mFramebuffer->myImage.mHeight;
    auto projectionPieces = TacMakeArray< m4 >(
      m4( // orient to bottom left
      1, 0, 0, 0,
      0, -1, 0, ( float )mRenderView->mFramebuffer->myImage.mHeight,
      0, 0, 1, 0,
      0, 0, 0, 1 ),
      m4( // convert to ndc
      sx, 0, 0, -1,
      0, sy, 0, -1,
      0, 0, 1, 0,
      0, 0, 0, 1 ) );
    v4 testVector( 130, 160, 0, 1 );
    bool testing = false;
    auto PrintTestVector = [ & ]()
    {
      std::cout
        << va(
        "Test vector: %.2f, %.2f, %.2f\n",
        testVector.x,
        testVector.y,
        testVector.z )
        << std::endl;
    };
    if( testing )
    {
      PrintTestVector();
    }
    m4 projection = m4::Identity();
    for( m4 projectionPiece : projectionPieces )
    {
      projection = projectionPiece * projection;
      if( testing )
      {
        testVector = projectionPiece * testVector;
        PrintTestVector();
      }
    }

    TacDefaultCBufferPerFrame perFrameData = {};
    perFrameData.mView = m4::Identity();
    perFrameData.mProjection = projection;
    TacDrawCall2 perFrame = {};
    perFrame.mRenderView = mRenderView;
    perFrame.mBlendState = TacUI2DCommonData::Instance->mBlendState;
    perFrame.mRasterizerState = TacUI2DCommonData::Instance->mRasterizerState;
    perFrame.mSamplerState = TacUI2DCommonData::Instance->mSamplerState;
    perFrame.mDepthState = TacUI2DCommonData::Instance->mDepthState;
    perFrame.mVertexFormat = TacUI2DCommonData::Instance->mFormat;
    perFrame.mUniformDst = TacUI2DCommonData::Instance->mPerFrame;
    perFrame.mUniformSrcc = TacTemporaryMemory( perFrameData );
    perFrame.mStackFrame = TAC_STACK_FRAME;
    TacRenderer::Instance->AddDrawCall( perFrame );

    for( TacUI2DDrawCall& uidrawCall : mDrawCall2Ds )
    {
      TacDrawCall2 drawCall2 = {};
      drawCall2.mUniformDst = TacUI2DCommonData::Instance->mPerObj;
      drawCall2.mIndexBuffer = mIndexes;
      drawCall2.mVertexBuffer = mVerts;
      drawCall2.mRenderView = mRenderView;
      drawCall2.mBlendState = TacUI2DCommonData::Instance->mBlendState;
      drawCall2.mRasterizerState = TacUI2DCommonData::Instance->mRasterizerState;
      drawCall2.mSamplerState = TacUI2DCommonData::Instance->mSamplerState;
      drawCall2.mDepthState = TacUI2DCommonData::Instance->mDepthState;
      drawCall2.mVertexFormat = TacUI2DCommonData::Instance->mFormat;
      drawCall2.mStartIndex = uidrawCall.mIIndexStart;
      drawCall2.mIndexCount = uidrawCall.mIIndexCount;
      drawCall2.mTextures = {
        uidrawCall.mTexture ?
        uidrawCall.mTexture :
        TacUI2DCommonData::Instance->m1x1White };
      drawCall2.mShader = uidrawCall.mShader;
      drawCall2.mUniformSrcc = uidrawCall.mUniformSource;
      drawCall2.mStackFrame = TAC_STACK_FRAME;
      TacRenderer::Instance->AddDrawCall( drawCall2 );
    }
  }

  mDrawCall2Ds.clear();
  mDefaultVertex2Ds.clear();
  mDefaultIndex2Ds.clear();

  TacRenderer::Instance->DebugBegin( "2d" );
  TacRenderer::Instance->RenderFlush();
  TacRenderer::Instance->DebugEnd();
}
TacUI2DState* TacUI2DDrawData::PushState()
{
  TacUI2DState state;
  state.mUI2DDrawData = this;
  if( !mStates.empty() )
    state = mStates.back();
  mStates.push_back( state );
  return &mStates.back();
}
void TacUI2DDrawData::PopState()
{
  TacAssert( mStates.size() );
  mStates.pop_back();
}

void TacUI2DState::Draw2DBox(
  float width,
  float height,
  v4 color,
  TacTexture* texture )
{
  if( width <= 0 || height <= 0 )
    return;
  auto oldVertexCount = ( int )mUI2DDrawData->mDefaultVertex2Ds.size();
  auto oldIndexCount = ( int )mUI2DDrawData->mDefaultIndex2Ds.size();
  auto offsets = TacMakeArray<v2>(
    v2( 0, 0 ),
    v2( 0, height ),
    v2( width, height ),
    v2( width, 0 ) );
  auto uvs = TacMakeArray<v2>(
    v2{ 0, 1 },
    v2{ 0, 0 },
    v2{ 1, 0 },
    v2{ 1, 1 } );
  const int vertexCount = 4;
  TacAssert( offsets.size() == vertexCount );
  TacAssert( uvs.size() == vertexCount );

  for( v2& offset : offsets )
    offset = ( mTransform * v3( offset, 1.0f ) ).xy();

  for( int iVert = 0; iVert < vertexCount; ++iVert )
  {
    TacUI2DVertex defaultVertex2D;
    defaultVertex2D.mGLTexCoord = uvs[ iVert ];
    defaultVertex2D.mPosition = offsets[ iVert ];
    mUI2DDrawData->mDefaultVertex2Ds.push_back( defaultVertex2D );
  }

  TacUI2DIndex indexes[] = { 0, 1, 2, 0, 2, 3 };
  for( int offset : indexes )
    mUI2DDrawData->mDefaultIndex2Ds.push_back( oldVertexCount + offset );

  TacDefaultCBufferPerObject perObjectData = {};
  perObjectData.World = m4::Identity();
  perObjectData.Color = color;

  TacUI2DDrawCall drawCall;
  drawCall.mIVertexStart = oldVertexCount;
  drawCall.mIVertexCount = vertexCount;
  drawCall.mIIndexStart = oldIndexCount;
  drawCall.mIIndexCount = 6;
  drawCall.mTexture = texture;
  drawCall.mShader = TacUI2DCommonData::Instance->mShader;
  drawCall.mUniformSource = TacTemporaryMemory( perObjectData );

  mUI2DDrawData->mDrawCall2Ds.push_back( drawCall );
}

void TacUI2DState::Draw2DText(
  TacLanguage defaultLanguage,
  int fontSize,
  const TacString& text,
  float* heightBetweenBaselines,
  v4 color,
  TacErrors& errors )
{
  TacFontStuff* fontStuff = TacUI2DCommonData::Instance->mFontStuff;
  const m3& transform = mTransform;

  TacVector< TacCodepoint > codepoints;
  TacUTF8Converter::Convert( text, codepoints, errors );
  TAC_HANDLE_ERROR( errors );
  auto oldIndexCount = ( int )mUI2DDrawData->mDefaultIndex2Ds.size();
  auto oldVertexCount = ( int )mUI2DDrawData->mDefaultVertex2Ds.size();
  float scale = ( float )fontSize / TacFontCellWidth;
  float lineHeight = scale * TacFontCellWidth;


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

  for( TacCodepoint codepoint : codepoints )
  {
    if( !codepoint )
      continue;

    if( TacIsAsciiCharacter( codepoint ) )
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


    TacFontAtlasCell* fontAtlasCell;
    fontStuff->GetCharacter( defaultLanguage, codepoint, &fontAtlasCell, errors );
    TAC_HANDLE_ERROR( errors );
    if( !fontAtlasCell )
      continue;

    const int quad_vertex_count = 4;
    auto startingIndex = ( int )mUI2DDrawData->mDefaultVertex2Ds.size();
    auto indexes = TacMakeArray< int >( 0, 1, 2, 0, 2, 3 );
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

    minY = TacMin( minY, fontAtlasCellY );
    maxY = TacMax( maxY, fontAtlasCellY + scaledBitmapHeight );
    maxX = TacMax( maxX, fontAtlasCellX + scaledBitmapWidth );

    for( const v2& glTexCoord : glTexCoords )
    {
      v3 position3 = transform * v3(
        fontAtlasCellX + glTexCoord.x * scaledBitmapWidth,
        fontAtlasCellY - glTexCoord.y * scaledBitmapHeight,
        1 );
      v2 position2 = position3.xy();

      TacUI2DVertex vert2D;
      vert2D.mPosition = position2;
      vert2D.mGLTexCoord.x = TacLerp(
        fontAtlasCell->mMinGLTexCoord.x,
        fontAtlasCell->mMaxGLTexCoord.x,
        glTexCoord.x );
      vert2D.mGLTexCoord.y = TacLerp(
        fontAtlasCell->mMinGLTexCoord.y,
        fontAtlasCell->mMaxGLTexCoord.y,
        glTexCoord.y );
      mUI2DDrawData->mDefaultVertex2Ds.push_back( vert2D );
    }

    runningX += fontAtlasCell->mUISpaceAdvanceWidth * scale;
  }

  TacDefaultCBufferPerObject perObjectData = {};
  perObjectData.World = m4::Identity();
  perObjectData.Color = color;

  TacUI2DDrawCall drawCall;
  drawCall.mIIndexCount = ( int )mUI2DDrawData->mDefaultIndex2Ds.size() - oldIndexCount;
  drawCall.mIIndexStart = oldIndexCount;
  drawCall.mIVertexCount = ( int )mUI2DDrawData->mDefaultVertex2Ds.size() - oldVertexCount;
  drawCall.mIVertexStart = oldVertexCount;
  drawCall.mTexture = fontStuff->mTexture;
  drawCall.mShader = TacUI2DCommonData::Instance->m2DTextShader;
  drawCall.mUniformSource = TacTemporaryMemory( perObjectData );
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
v2 TacUI2DDrawData::CalculateTextSize( const TacString& text, int fontSize )
{
  TacVector< TacCodepoint > codepoints;

  // ignored
  TacErrors errors;

  TacUTF8Converter::Convert( text, codepoints, errors );
  return CalculateTextSize( codepoints, fontSize );
}

v2 TacUI2DDrawData::CalculateTextSize( const TacVector< TacCodepoint >& codepoints, int fontSize )
{
  return CalculateTextSize( codepoints.data(), ( int )codepoints.size(), fontSize );
}
v2 TacUI2DDrawData::CalculateTextSize( const TacCodepoint* codepoints, int codepointCount, int fontSize )
{
  float lineWidthUISpaceMax = 0;
  float lineWidthUISpace = 0;
  float xUISpace = 0;


  TacFontStuff* fontStuff = TacUI2DCommonData::Instance->mFontStuff;
  TacLanguage defaultLanguage = TacLanguage::English;
  TacFontFile* fontFile = fontStuff->mDefaultFonts[ defaultLanguage ];

  int lineCount = 1;

  auto AccountForLine = [&]()
  {
    lineWidthUISpaceMax = TacMax( lineWidthUISpaceMax, lineWidthUISpace );

    // if the string ends with a space ( ' ' )
    // ( bitmap width is 0, advance width is nonzero )
    lineWidthUISpaceMax = TacMax( lineWidthUISpaceMax, xUISpace );
  };

  for( int iCodepoint = 0; iCodepoint < codepointCount; ++iCodepoint )
  {
   TacCodepoint codepoint = codepoints[ iCodepoint ];
    if( !codepoint )
      continue;

    if( TacIsAsciiCharacter( codepoint ) )
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

    TacFontAtlasCell* fontAtlasCell = nullptr;

    // ignored...
    TacErrors errors;

    fontStuff->GetCharacter( defaultLanguage, codepoint, &fontAtlasCell, errors );

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
  textSize *= ( float )fontSize / ( float )TacFontCellWidth;

  return textSize;
}

void TacUI2DDrawData::AddBox( v2 mini, v2 maxi, v4 color, const TacTexture* texture, const TacImGuiRect* clipRect )
{
  if( clipRect )
  {
    mini = { TacMax( mini.x, clipRect->mMini.x ),
             TacMax( mini.y, clipRect->mMini.y ) };
    maxi = { TacMin( maxi.x, clipRect->mMaxi.x ),
             TacMin( maxi.y, clipRect->mMaxi.y ) };
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
  TacUI2DVertex* defaultVertex2D = &mDefaultVertex2Ds[ iVert ];

  defaultVertex2D->mPosition = { mini.x, mini.y };
  defaultVertex2D++;
  defaultVertex2D->mPosition = { mini.x, maxi.y };
  defaultVertex2D++;
  defaultVertex2D->mPosition = { maxi.x, maxi.y };
  defaultVertex2D++;
  defaultVertex2D->mPosition = { maxi.x, mini.y };

  TacDefaultCBufferPerObject perObjectData = {};
  perObjectData.World = m4::Identity();
  perObjectData.Color = color;

  TacUI2DDrawCall drawCall;
  drawCall.mIVertexStart = iVert;
  drawCall.mIVertexCount = 4;
  drawCall.mIIndexStart = iIndex;
  drawCall.mIIndexCount = 6;
  drawCall.mTexture = texture;
  drawCall.mShader = TacUI2DCommonData::Instance->mShader;
  drawCall.mUniformSource = TacTemporaryMemory( perObjectData );

  mDrawCall2Ds.push_back( drawCall );
}

void TacUI2DDrawData::AddLine( v2 p0, v2 p1, float radius, v4 color )
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
  mDefaultVertex2Ds[ iVert + 1 ].mPosition = p0 - dphatccw * radius;
  mDefaultVertex2Ds[ iVert + 2 ].mPosition = p1 + dphatccw * radius;
  mDefaultVertex2Ds[ iVert + 3 ].mPosition = p1 - dphatccw * radius;
  mDefaultIndex2Ds.resize( iIndex + 6 );
  mDefaultIndex2Ds[ iIndex + 0 ] = iVert + 0;
  mDefaultIndex2Ds[ iIndex + 1 ] = iVert + 1;
  mDefaultIndex2Ds[ iIndex + 2 ] = iVert + 2;
  mDefaultIndex2Ds[ iIndex + 3 ] = iVert + 1;
  mDefaultIndex2Ds[ iIndex + 4 ] = iVert + 3;
  mDefaultIndex2Ds[ iIndex + 5 ] = iVert + 2;

  TacDefaultCBufferPerObject perObjectData = {};
  perObjectData.World = m4::Identity();
  perObjectData.Color = color;

  TacUI2DDrawCall drawCall;
  drawCall.mIVertexStart = iVert;
  drawCall.mIVertexCount = 4;
  drawCall.mIIndexStart = iIndex;
  drawCall.mIIndexCount = 6;
  drawCall.mTexture = nullptr;
  drawCall.mShader = TacUI2DCommonData::Instance->mShader;
  drawCall.mUniformSource = TacTemporaryMemory( perObjectData );

  mDrawCall2Ds.push_back( drawCall );
}

//void TacUI2DDrawData::AddPolyFill( const TacVector< v2 >& points, v4 color )
//{
//}
//void TacUI2DDrawData::AddSquiggle( const TacVector< v2 >& inputPoints, float width, v4 color )
//{
//  int iVert = mDefaultVertex2Ds.size();
//  int iIndex = mDefaultIndex2Ds.size();
//
//  TacVector< v2 > outputPoints;
//  outputPoints.resize( 2 * inputPoints.size() - 2 );
//
//  mDefaultVertex2Ds.resize( iVert + outputPoints.size() );
//
//  TacDefaultVertex2D* defaultVertex2D = &mDefaultVertex2Ds[ iVert ];
//
//
//  TacVector< v2 > normals;
//}

void TacUI2DDrawData::AddText( v2 textPos, int fontSize, const TacString& utf8, v4 color, const TacImGuiRect* clipRect )
{
  if( utf8.empty() )
    return;

  // ignored
  TacErrors errors;

  TacVector< TacCodepoint > codepoints;
  TacUTF8Converter::Convert( utf8, codepoints, errors );

  TacFontStuff* fontStuff = TacUI2DCommonData::Instance->mFontStuff;
  TacLanguage defaultLanguage = TacLanguage::English;
  TacFontFile* fontFile = fontStuff->mDefaultFonts[ defaultLanguage ];

  float scaleUIToPx = ( float )fontSize / ( float )TacFontCellWidth;
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

  for( TacCodepoint codepoint : codepoints )
  {
    if( !codepoint )
      continue;

    if( TacIsAsciiCharacter( codepoint ) )
    {
      if( codepoint == '\n' )
      {
        xPxCursor = textPos.x;
        yPxBaseline += descentPx + linegapPx + ascentPx;
      }
      if( codepoint == '\r' )
        continue;
    }

    TacFontAtlasCell* fontAtlasCell = nullptr;
    fontStuff->GetCharacter( defaultLanguage, codepoint, &fontAtlasCell, errors );
    // ^ ignore errors...

    if( !fontAtlasCell )
      continue;

    OnDestruct(
      // Even if we don't draw a glyph, we still need to advance the cursor,
      // For example if our current codepoint represents whitespace
      xPxCursor += fontAtlasCell->mAdvanceWidth * scaleFontToPx;
    );

    auto startingIndex = ( TacUI2DIndex )mDefaultVertex2Ds.size();
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
      glyphMinX = TacMax( glyphMinX, clipRect->mMini.x );
      glyphMaxX = TacMin( glyphMaxX, clipRect->mMaxi.x );
      glyphMinY = TacMax( glyphMinY, clipRect->mMini.y );
      glyphMaxY = TacMin( glyphMaxY, clipRect->mMaxi.y );
    }

    if( glyphMinX == glyphMaxX || glyphMinY == glyphMaxY )
      continue;
    TacAssert( glyphMaxX > glyphMinX );
    TacAssert( glyphMaxY > glyphMinY );

    // todo: compute clipped uvs

    mDefaultVertex2Ds.resize( startingIndex + 4 );
    TacUI2DVertex* defaultVertex2D = &mDefaultVertex2Ds[ startingIndex ];

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

  TacDefaultCBufferPerObject perObjectData = {};
  perObjectData.World = m4::Identity();
  perObjectData.Color = color;

  TacUI2DDrawCall drawCall;
  drawCall.mIIndexCount = indexCount;
  drawCall.mIIndexStart = indexStart;
  drawCall.mIVertexCount = vertexCount;
  drawCall.mIVertexStart = vertexStart;
  drawCall.mTexture = fontStuff->mTexture;
  drawCall.mShader = TacUI2DCommonData::Instance->m2DTextShader;
  drawCall.mUniformSource = TacTemporaryMemory( perObjectData );
  mDrawCall2Ds.push_back( drawCall );
}

void TacUI2DState::Translate( v2 pos )
{
  Translate( pos.x, pos.y );
}
void TacUI2DState::Translate( float x, float y )
{
  mTransform = mTransform * M3Translate( x, y );
}
