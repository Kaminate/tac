
#include "common/tacRenderer.h"
#include "common/tacPreprocessor.h"
#include "common/tacAlgorithm.h"
#include "common/tacShell.h"
#include "common/tacOS.h"

//#include "imgui.h"

const char* TacGetSemanticName( TacAttribute attribType )
{
  switch( attribType )
  {
  case TacAttribute::Position: return "POSITION";
  case TacAttribute::Normal: return "NORMAL";
  case TacAttribute::Texcoord: return "TEXCOORD";
  case TacAttribute::Color: return "COLOR";
  case TacAttribute::BoneIndex: return "BONEINDEX";
  case TacAttribute::BoneWeight: return "BONEWEIGHT";
  case TacAttribute::Coeffs: return "COEFFS";
    TacInvalidDefaultCase( attribType );
  }
  return nullptr;
}

TacString TacToString( TacRendererType rendererType )
{
  switch( rendererType )
  {
  case TacRendererType::Vulkan: return "Vulkan";
  case TacRendererType::OpenGL4: return "OpenGL4";
  case TacRendererType::DirectX12: return "DirectX12";
    TacInvalidDefaultCase( rendererType );
  }
  TacInvalidCodePath;
  return "";
}
v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated )
{
  return {
    colorAlphaUnassociated.x * colorAlphaUnassociated.w,
    colorAlphaUnassociated.y * colorAlphaUnassociated.w,
    colorAlphaUnassociated.z * colorAlphaUnassociated.w,
    colorAlphaUnassociated.w };
}


int TacFormat::CalculateTotalByteCount() const
{
  return mElementCount * mPerElementByteCount;
}

TacVertexDeclarations TacDefaultVertex2D::sVertexDeclarations = []() ->TacVertexDeclarations {
  TacVertexDeclaration posData;
  posData.mAlignedByteOffset = TacOffsetOf( TacDefaultVertex2D, mPosition );
  posData.mAttribute = TacAttribute::Position;
  posData.mTextureFormat = formatv2;
  TacVertexDeclaration uvData;
  uvData.mAlignedByteOffset = TacOffsetOf( TacDefaultVertex2D, mGLTexCoord );
  uvData.mAttribute = TacAttribute::Texcoord;
  uvData.mTextureFormat = formatv2;
  return { posData, uvData };
}( );
TacVertexDeclarations TacDefaultVertexColor::sVertexDeclarations = []() ->TacVertexDeclarations {
  TacVertexDeclaration positionData;
  positionData.mAttribute = TacAttribute::Position;
  positionData.mTextureFormat = formatv3;
  positionData.mAlignedByteOffset = TacOffsetOf( TacDefaultVertexColor, mPosition );
  TacVertexDeclaration colorData;
  colorData.mAttribute = TacAttribute::Color;
  colorData.mTextureFormat = formatv3;
  colorData.mAlignedByteOffset = TacOffsetOf( TacDefaultVertexColor, mColor );
  return { positionData, colorData };
} ( );


TacRenderer::~TacRenderer()
{
  for( TacRendererResource* rendererResource : mRendererResources )
  {
    TacOS::Instance->DebugAssert(
      "Resource leaked: " + rendererResource->mName,
      TAC_STACK_FRAME );
  }
}

void TacRenderer::LoadDefaultGraphicsObjects( TacErrors& errors )
{
  typedef uint16_t IndexType;
  TacFormat indexFormat;
  indexFormat.mPerElementByteCount = sizeof( IndexType );
  indexFormat.mElementCount = 1;
  indexFormat.mPerElementDataType = TacGraphicsType::uint;
  TacVector< TacDefaultVertex2D > textData;
  TacVector< IndexType > indexes = { 0, 1, 2, 0, 2, 3 };
  //const int quad_index_count = 6;
  const int quad_vertex_count = 4;
  const v2 ndcPositions[ quad_vertex_count ] = { v2( -1, -1 ), v2( 1, -1 ), v2( 1, 1 ), v2( -1, 1 ) };
  const v2 glTexCoords[ quad_vertex_count ] = { v2( 0, 0 ), v2( 1, 0 ), v2( 1, 1 ), v2( 0, 1 ) };
  for( int i = 0; i < quad_vertex_count; ++i )
  {
    const v2 ndcPosition = ndcPositions[ i ];
    const v2 glTexCoord = glTexCoords[ i ];
    TacDefaultVertex2D vert2D = {};
    vert2D.mPosition = ndcPosition;
    vert2D.mGLTexCoord = glTexCoord;
    textData.push_back( vert2D );
  }
  TacVertexBufferData vertexBufferData;
  vertexBufferData.access = TacAccess::Default;
  vertexBufferData.mStrideBytesBetweenVertexes = sizeof( TacDefaultVertex2D );
  vertexBufferData.mNumVertexes = ( int )textData.size();
  vertexBufferData.optionalData = textData.data();
  vertexBufferData.mName = "default vertz";
  vertexBufferData.mStackFrame = TAC_STACK_FRAME;

  AddVertexBuffer( &m2DNDCQuadVB, vertexBufferData, errors );
  TAC_HANDLE_ERROR( errors );

  TacIndexBufferData data2DNDCQuadIB;
  data2DNDCQuadIB.access = TacAccess::Default;
  data2DNDCQuadIB.data = indexes.data();
  data2DNDCQuadIB.dataType = indexFormat;
  data2DNDCQuadIB.indexCount = ( int )indexes.size();
  data2DNDCQuadIB.mName = "2d ndc quad ib";
  data2DNDCQuadIB.mStackFrame = TAC_STACK_FRAME;
  AddIndexBuffer( &m2DNDCQuadIB, data2DNDCQuadIB, errors );
  TAC_HANDLE_ERROR( errors );


  uint8_t imageData[] = { 255, 255, 255, 255 };
  TacImage image;
  image.mData = &imageData;
  image.mFormat.mPerElementByteCount = 1;
  image.mFormat.mElementCount = 4;
  image.mFormat.mPerElementDataType = TacGraphicsType::unorm;
  image.mWidth = 1;
  image.mHeight = 1;
  image.mPitch = 4;
  TacTextureData data1x1White;
  data1x1White.access = TacAccess::Static;
  data1x1White.binding = { TacBinding::ShaderResource };
  data1x1White.cpuAccess = {};
  data1x1White.mName = "1x1 white";
  data1x1White.mStackFrame = TAC_STACK_FRAME;
  data1x1White.myImage = image;
  AddTextureResource( &m1x1White, data1x1White, errors );
  TAC_HANDLE_ERROR( errors );

  TacRasterizerStateData rasterizerStateFrontCCWCullBackData;
  rasterizerStateFrontCCWCullBackData.cullMode = TacCullMode::Back;
  rasterizerStateFrontCCWCullBackData.fillMode = TacFillMode::Solid;
  rasterizerStateFrontCCWCullBackData.frontCounterClockwise = true;
  rasterizerStateFrontCCWCullBackData.mName = "rasterizer state front CCW Cull Back";
  rasterizerStateFrontCCWCullBackData.mStackFrame = TAC_STACK_FRAME;
  rasterizerStateFrontCCWCullBackData.multisample = false;
  rasterizerStateFrontCCWCullBackData.scissor = true;
  AddRasterizerState( &mRasterizerStateFrontCCWCullBack, rasterizerStateFrontCCWCullBackData, errors );
  TAC_HANDLE_ERROR( errors );

  TacRasterizerStateData rasterizerStateNoCullData;
  rasterizerStateNoCullData.cullMode = TacCullMode::None;
  rasterizerStateNoCullData.fillMode = TacFillMode::Solid;
  rasterizerStateNoCullData.frontCounterClockwise = true;
  rasterizerStateNoCullData.mName = "no cull";
  rasterizerStateNoCullData.mStackFrame = TAC_STACK_FRAME;
  rasterizerStateNoCullData.multisample = false;
  rasterizerStateNoCullData.scissor = true;
  AddRasterizerState( &mRasterizerStateNoCull, rasterizerStateNoCullData, errors );
  TAC_HANDLE_ERROR( errors );

  TacShaderData data3dVertexColorShaderData;
  data3dVertexColorShaderData.mShaderPath = "3DVertexColor";
  data3dVertexColorShaderData.mStackFrame = TAC_STACK_FRAME;
  data3dVertexColorShaderData.mName = "3d color";
  AddShader( &m3DVertexColorShader, data3dVertexColorShaderData, errors );
  TAC_HANDLE_ERROR( errors );

  TacCBufferData cBufferPerFrameData;
  cBufferPerFrameData.mName = "cbuffer per frame";
  cBufferPerFrameData.mStackFrame = TAC_STACK_FRAME;
  cBufferPerFrameData.shaderRegister = CBufferPerFrame::shaderRegister;
  cBufferPerFrameData.byteCount = sizeof( CBufferPerFrame );
  AddConstantBuffer( &mCBufferPerFrame, cBufferPerFrameData, errors );
  TAC_HANDLE_ERROR( errors );

  TacCBufferData cBufferPerObjectData;
  cBufferPerObjectData.mName = "cbuffer per object";
  cBufferPerObjectData.mStackFrame = TAC_STACK_FRAME;
  cBufferPerObjectData.shaderRegister = CBufferPerObject::shaderRegister;
  cBufferPerObjectData.byteCount = sizeof( CBufferPerObject );
  AddConstantBuffer( &mCBufferPerObject, cBufferPerObjectData, errors );
  TAC_HANDLE_ERROR( errors );

  for( auto shaderType : { TacShaderType::Vertex, TacShaderType::Fragment } )
  {
    for( auto cbuffer : { mCBufferPerObject, mCBufferPerFrame } )
    {
      AddCbufferToShader( m3DVertexColorShader, cbuffer, shaderType );
    }
  }


  TacDepthStateData depthStateData;
  depthStateData.depthFunc = TacDepthFunc::Less;
  depthStateData.depthTest = true;
  depthStateData.depthWrite = true;
  depthStateData.mName = "depth less";
  depthStateData.mStackFrame = TAC_STACK_FRAME;
  AddDepthState( &mDepthLess, depthStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacVertexFormatData vertexColorFormatDataa;
  vertexColorFormatDataa.mName = "vertex color format";
  vertexColorFormatDataa.mStackFrame = TAC_STACK_FRAME;
  vertexColorFormatDataa.shader = m3DVertexColorShader;
  vertexColorFormatDataa.vertexFormatDatas = TacDefaultVertexColor::sVertexDeclarations;
  AddVertexFormat( &mVertexColorFormat, vertexColorFormatDataa, errors );
  TAC_HANDLE_ERROR( errors );

  mDebugDrawVertMax = mDebugDrawBytes / sizeof( TacDefaultVertexColor );

  TacVertexBufferData vbData;
  vbData.access = TacAccess::Dynamic;
  vbData.mNumVertexes = mDebugDrawVertMax;
  vbData.mStrideBytesBetweenVertexes = sizeof( TacDefaultVertexColor );
  vbData.optionalData = nullptr;
  vbData.mName = "default vertz";
  vbData.mStackFrame = TAC_STACK_FRAME;
  AddVertexBuffer( &mDebugLineVB, vbData, errors );
  TAC_HANDLE_ERROR( errors );




  TacShaderData shaderData2D;
  shaderData2D.mName = "2D default shader";
  shaderData2D.mShaderPath = "2D";
  shaderData2D.mStackFrame = TAC_STACK_FRAME;
  AddShader( &m2DShader, shaderData2D, errors );
  TAC_HANDLE_ERROR( errors );
  for( auto shaderType : { TacShaderType::Vertex, TacShaderType::Fragment } )
  {
    for( auto cbuffer : { mCBufferPerFrame, mCBufferPerObject } )
    {
      AddCbufferToShader( m2DShader, cbuffer, shaderType );
    }
  }
  AddTexture( "atlas", m2DShader, TacShaderType::Fragment, 0 );
  AddSampler( "linearSampler", m2DShader, TacShaderType::Fragment, 0 );


  TacShaderData shaderData2DText;
  shaderData2DText.mName = "2D text shader";
  shaderData2DText.mShaderPath = "2DText";
  shaderData2DText.mStackFrame = TAC_STACK_FRAME;
  AddShader( &m2DTextShader, shaderData2DText, errors );
  TAC_HANDLE_ERROR( errors );
  for( auto shaderType : { TacShaderType::Vertex, TacShaderType::Fragment } )
  {
    for( auto cbuffer : { mCBufferPerFrame, mCBufferPerObject } )
    {
      AddCbufferToShader( m2DTextShader, cbuffer, shaderType );
    }
  }
  AddTexture( "atlas", m2DTextShader, TacShaderType::Fragment, 0 );
  AddSampler( "linearSampler", m2DTextShader, TacShaderType::Fragment, 0 );


  TacSamplerStateData mSamplerStateLinearWrapData;
  mSamplerStateLinearWrapData.compare = TacComparison::Never;
  mSamplerStateLinearWrapData.u = TacAddressMode::Wrap;
  mSamplerStateLinearWrapData.v = TacAddressMode::Wrap;
  mSamplerStateLinearWrapData.w = TacAddressMode::Wrap;
  mSamplerStateLinearWrapData.filter = TacFilter::Linear;
  mSamplerStateLinearWrapData.mName = "linear wrap";
  mSamplerStateLinearWrapData.mStackFrame = TAC_STACK_FRAME;
  AddSamplerState( &mSamplerStateLinearWrap, mSamplerStateLinearWrapData, errors );
  TAC_HANDLE_ERROR( errors );

  TacBlendStateData alphaBlendStateData;
  alphaBlendStateData.srcRGB = TacBlendConstants::One;
  alphaBlendStateData.dstRGB = TacBlendConstants::OneMinusSrcA;
  alphaBlendStateData.blendRGB = TacBlendMode::Add;
  alphaBlendStateData.srcA = TacBlendConstants::One;
  alphaBlendStateData.dstA = TacBlendConstants::OneMinusSrcA;
  alphaBlendStateData.blendA = TacBlendMode::Add;
  alphaBlendStateData.mName = "alpha blend";
  alphaBlendStateData.mStackFrame = TAC_STACK_FRAME;
  AddBlendState( &mAlphaBlendState, alphaBlendStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacDepthStateData noDepthReadOrWriteData;
  noDepthReadOrWriteData.depthTest = false;
  noDepthReadOrWriteData.depthWrite = false;
  noDepthReadOrWriteData.depthFunc = TacDepthFunc::Less;
  noDepthReadOrWriteData.mName = "no depth read/write";
  noDepthReadOrWriteData.mStackFrame = TAC_STACK_FRAME;
  AddDepthState( &mNoDepthReadOrWrite, noDepthReadOrWriteData, errors );
  TAC_HANDLE_ERROR( errors );

  TacVertexFormatData defaultVertex2DFormatDataa;
  defaultVertex2DFormatDataa.vertexFormatDatas = TacDefaultVertex2D::sVertexDeclarations;
  defaultVertex2DFormatDataa.shader = m2DShader;
  defaultVertex2DFormatDataa.mName = "2D vertex format";
  defaultVertex2DFormatDataa.mStackFrame = TAC_STACK_FRAME;
  AddVertexFormat( &mDefaultVertex2DFormat, defaultVertex2DFormatDataa, errors );
  TAC_HANDLE_ERROR( errors );
}

void TacRenderer::UnloadDefaultGraphicsObjects()
{
  delete mAlphaBlendState;
  delete mCBufferPerFrame;
  delete mCBufferPerObject;
  delete mDepthLess;
  delete mNoDepthReadOrWrite;
  delete mRasterizerStateFrontCCWCullBack;
  delete mRasterizerStateNoCull;
  delete mSamplerStateLinearWrap;
  delete m3DVertexColorShader;
  delete m2DShader;
  delete mDebugLineVB;
  delete m2DNDCQuadVB;
  delete mVertexColorFormat;
  delete mDefaultVertex2DFormat;
  delete m1x1White;
  delete m2DNDCQuadIB;
}

void TacRenderer::DebugImgui()
{
  //if( !ImGui::CollapsingHeader( "Renderer" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //if( ImGui::CollapsingHeader( "Shaders" ) )
  //{
  //  ImGui::Indent();
  //  OnDestruct( ImGui::Unindent() );
  //  auto reloadAll = ImGui::Button( "Reload all shaders" );
  //  TacVector< TacShader*>shaders;
  //  GetShaders( shaders );
  //  for( auto shader : shaders )
  //  {
  //    auto reloadShader = reloadAll || ImGui::Button( shader->mName.c_str() );
  //    if( !reloadShader )
  //      continue;
  //    ReloadShader( shader, mShaderReloadErrors );
  //  }
  //  ImGui::Text( mShaderReloadErrors.mMessage );
  //}

  //if( ImGui::CollapsingHeader( "Textures" ) )
  //{
  //  ImGui::Indent();
  //  OnDestruct( ImGui::Unindent() );
  //  TacVector< TacTexture* > textures;
  //  GetTextures( textures );
  //  for( auto texture : textures )
  //  {
  //    ImGui::PushID( texture );
  //    OnDestruct( ImGui::PopID() );
  //    if( ImGui::CollapsingHeader( texture->mName.c_str() ) )
  //    {
  //      ImGui::Indent();
  //      OnDestruct( ImGui::Unindent() );
  //      ImGui::Text( "Width: %ipx", texture->myImage.mWidth );
  //      ImGui::Text( "Height: %ipx", texture->myImage.mHeight );
  //      float w = 0.9f * ImGui::GetContentRegionAvailWidth();
  //      float h = w / texture->GetAspect();
  //      auto id = texture->GetImguiTextureID();
  //      auto size = ImVec2( w, h );
  //      ImGui::Image( id, size );
  //    }
  //  }
  //}
}

const float sizeInMagicUISpaceUnits = 1024.0f;

// Make this static?
void TacRenderer::GetUIDimensions(
  TacTexture* texture,
  float* width,
  float* height )
{
  float aspect = texture->GetAspect();
  float scaleWidth = 1;
  float scaleHeight = 1;
  if( aspect > 1 )
    scaleWidth = aspect;
  else
    scaleHeight = 1.0f / aspect;
  *width = scaleWidth * sizeInMagicUISpaceUnits;
  *height = scaleHeight * sizeInMagicUISpaceUnits;
}

void TacRenderer::Render2D(
  const TacVector< TacDefaultVertex2D >& defaultVertex2Ds,
  const TacVector< TacDefaultIndex2D >& defaultIndex2Ds,
  const TacVector< TacDrawCall >& drawCalls,
  TacErrors& errors )
{
  DebugBegin( "Draw 2D" );
  OnDestruct( DebugEnd() );

  if( defaultIndex2Ds.empty() ||
    defaultVertex2Ds.empty() )
    return;

  TacTexture* fboTexture = mCurRenderTargets[ 0 ];
  //float aspect = fboTexture->GetAspect();

  //SetBlendState( mAlphaBlendState );
  //SetRasterizerState( mRasterizerStateFrontCCWCullBack );
  //SetVertexFormat( mDefaultVertex2DFormat );
  //SetDepthState( mNoDepthReadOrWrite );
  SetPrimitiveTopology( TacPrimitive::TriangleList );
  //SetScissorRect( 0, 0, ( float )fboTexture->mWidth, ( float )fboTexture->mHeight );

  float uiWidth;
  float uiHeight;
  GetUIDimensions( fboTexture, &uiWidth, &uiHeight );
  float xUItoNDC = 2.0f / uiWidth;
  float yUItoNDC = 2.0f / uiHeight;

  //m4 world = M4Scale( xUItoNDC, yUItoNDC, 1 );
  //m4 identity = m4::Identity();
  //SendUniform( "View", identity.data() );
  //SendUniform( "Projection", identity.data() );
  //SendUniform( "World", world.data() );

  TacFormat indexFormat;
  indexFormat.mPerElementByteCount = sizeof( TacDefaultIndex2D );
  indexFormat.mElementCount = 1;
  indexFormat.mPerElementDataType = TacGraphicsType::uint;

  TacVertexBufferData vbData;
  vbData.access = TacAccess::Default;
  vbData.mNumVertexes = ( int )defaultVertex2Ds.size();
  vbData.mStrideBytesBetweenVertexes = sizeof( TacDefaultVertex2D );
  vbData.optionalData = ( void* )defaultVertex2Ds.data();
  TacVertexBuffer* vertexBuffer;
  AddVertexBuffer( &vertexBuffer, vbData, errors );
  TAC_HANDLE_ERROR( errors );

  TacIndexBuffer* indexBuffer;
  TacIndexBufferData indexBufferData;
  indexBufferData.access = TacAccess::Default;
  indexBufferData.data = defaultIndex2Ds.data();
  indexBufferData.dataType = indexFormat;
  indexBufferData.indexCount = ( int )defaultIndex2Ds.size();
  indexBufferData.mName = "my ibuf";
  indexBufferData.mStackFrame = TAC_STACK_FRAME;
  AddIndexBuffer( &indexBuffer, indexBufferData, errors );
  TAC_HANDLE_ERROR( errors );

  //SetVertexBuffer( vertexBuffer );
  //SetIndexBuffer( indexBuffer );
  int startingIndex = 0;
  for( const TacDrawCall& drawCall : drawCalls )
  {
    TacTexture* texture = drawCall.mTexture;
    if( !texture )
      texture = m1x1White;
    //TacShader* shader = drawCall.mIsText ? m2DTextShader : m2DShader;
    //SetActiveShader( shader );
    SetSamplerState( "linearSampler", mSamplerStateLinearWrap );
    SetTexture( "atlas", texture );
    //v4 color = ToColorAlphaPremultiplied( drawCall.mColor );
    //SendUniform( "Color", color.data() );
    Apply();
    v2 scissorMin = {};
    v2 scissorMax = { ( float )fboTexture->myImage.mWidth, ( float )fboTexture->myImage.mHeight };
    if( drawCall.mScissorTest )
    {
      // note: y min/max are flipped because in uispace y increases upwards but in scissor space y increases downwards
      float xMin = ( drawCall.mScissorRectMinUISpace.x * xUItoNDC + 1.0f ) * 0.5f * fboTexture->myImage.mWidth;
      float xMax = ( drawCall.mScissorRectMaxUISpace.x * xUItoNDC + 1.0f ) * 0.5f * fboTexture->myImage.mWidth;
      float yMin = ( -drawCall.mScissorRectMaxUISpace.y * yUItoNDC + 1.0f ) * 0.5f * fboTexture->myImage.mHeight;
      float yMax = ( -drawCall.mScissorRectMinUISpace.y * yUItoNDC + 1.0f ) * 0.5f * fboTexture->myImage.mHeight;
      scissorMin = { xMin, yMin };
      scissorMax = { xMax, yMax };
    }
    // hmm...
    //SetScissorRect( scissorMin.x, scissorMin.y, scissorMax.x, scissorMax.y );
    DrawIndexed( drawCall.mIndexCount, startingIndex, 0 );
    startingIndex += drawCall.mIndexCount;
  }

  delete vertexBuffer;
  delete indexBuffer;
}

void TacRenderer::DebugDraw(
  m4 world_to_view,
  m4 view_to_clip,
  TacTexture* texture,
  TacDepthBuffer* depth,
  const TacVector< TacDefaultVertexColor >& mDebugDrawVerts,
  TacErrors& errors )
{
  return;
  DebugBegin( "Debug Draw" );
  OnDestruct( DebugEnd() );
  //SetRenderTarget( texture, depth );

  //auto local_to_world = m4::Identity();
  //v4 objectColor( 1, 1, 1, 1 );

  //SetDepthState( mDepthLess );
  //SetActiveShader( m3DVertexColorShader );
  //SendUniform( CBufferPerFrame::name_proj(), view_to_clip.data() );
  //SendUniform( CBufferPerFrame::name_view(), world_to_view.data() );
  //SendUniform( CBufferPerObject::name_world(), local_to_world.data() );
  //SendUniform( CBufferPerObject::name_color(), objectColor.data() );
  Apply();
  mDebugLineVB->Overwrite( ( void* )mDebugDrawVerts.data(), mDebugDrawVerts.size() * sizeof( TacDefaultVertexColor ), errors );
  TAC_HANDLE_ERROR( errors );
  //SetVertexBuffer( mDebugLineVB );
  //SetVertexFormat( mVertexColorFormat );
  SetPrimitiveTopology( TacPrimitive::LineList );

  auto viewportW = ( float )texture->myImage.mWidth;
  auto viewportH = ( float )texture->myImage.mHeight;
  //SetViewport(
  //  0, // x rel bot left
  //  0, // y rel bot left
  //  viewportW,
  //  viewportH );
  DrawNonIndexed( ( int )mDebugDrawVerts.size() );
  DebugEnd();
}

void TacDrawCall::DebugImgui()
{

  //ImGui::PushID( this );
  //OnDestruct( ImGui::PopID() );

  //if( !ImGui::CollapsingHeader( "Draw call" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );

  //if( mTexture )
  //{
  //  ImGui::Text( "Texture: " + mTexture->mName );
  //  float w = 0.9f * ImGui::GetContentRegionAvailWidth();
  //  float h = w / mTexture->GetAspect();
  //  ImGui::Image( mTexture->GetImguiTextureID(), ImVec2( w, h ) );
  //}
  //ImGui::ColorEdit4( "color", mColor.data() );
  //ImGui::Checkbox( "scissor test", &mScissorTest );
  //ImGui::Checkbox( "scissor debug", &mScissorDebugging );
  //ImGui::DragFloat2( "scissor min", mScissorRectMinUISpace.data() );
  //ImGui::DragFloat2( "scissor max", mScissorRectMaxUISpace.data() );
  //ImGui::DragInt( "Index count", &mIndexCount );
}

void TacRendererFactory::CreateRendererOuter( TacRenderer** renderer )
{
  CreateRenderer( renderer );
  ( *renderer )->mName = mRendererName;
}

TacVector< TacRendererFactory* >& TacRendererFactory::GetRegistry()
{
  static TacVector< TacRendererFactory* > registry;
  return registry;
}

void TacRenderer::RemoveRendererResource( TacRendererResource* rendererResource )
{
  bool found = TacContains( mRendererResources, rendererResource );
  TacAssert( found );
  mRendererResources.erase( rendererResource );
  delete rendererResource;
}

