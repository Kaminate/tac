#include "tacSkyboxPresentation.h"
#include "common/tacString.h"
#include "common/tacMemory.h"
#include "common/tacDesktopWindow.h"
#include "common/tacCamera.h"
#include "common/tacErrorHandling.h"
#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/assetmanagers/tacModelAssetManager.h"
#include "common/graphics/tacRenderer.h"

TacSkyboxPresentation::~TacSkyboxPresentation()
{
  TacRenderer* renderer = TacRenderer::Instance;
  TacRenderer::Instance->RemoveRendererResource( mShader );
  TacRenderer::Instance->RemoveRendererResource( mVertexFormat );
  TacRenderer::Instance->RemoveRendererResource( mPerFrame  );
}
void TacSkyboxPresentation::Init( TacErrors& errors )
{
  TacRenderer* renderer = TacRenderer::Instance;

  TacCBufferData cBufferDataPerFrame = {};
  cBufferDataPerFrame.mName = "skybox per frame";
  cBufferDataPerFrame.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerFrame.shaderRegister = 0;
  cBufferDataPerFrame.byteCount = sizeof( TacDefaultCBufferPerFrame );
  TacRenderer::Instance->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
  TAC_HANDLE_ERROR( errors );

  TacShaderData shaderData = {};
  shaderData.mShaderPath = "Skybox";
  shaderData.mCBuffers = { mPerFrame };
  shaderData.mStackFrame = TAC_STACK_FRAME;
  shaderData.mName = "skybox";
  TacRenderer::Instance->AddShader( &mShader, shaderData, errors );
  TAC_HANDLE_ERROR( errors );

  TacVertexDeclaration pos;
  pos.mAlignedByteOffset = 0;
  pos.mAttribute = TacAttribute::Position;
  pos.mTextureFormat.mElementCount = 3;
  pos.mTextureFormat.mPerElementByteCount = sizeof( float );
  pos.mTextureFormat.mPerElementDataType = TacGraphicsType::real;

  TacVertexFormatData vertexFormatData = {};
  vertexFormatData.mName = "skybox";
  vertexFormatData.mStackFrame = TAC_STACK_FRAME;
  vertexFormatData.shader = mShader;
  vertexFormatData.vertexFormatDatas = { pos };
  TacRenderer::Instance->AddVertexFormat( &mVertexFormat, vertexFormatData, errors );
  TAC_HANDLE_ERROR( errors );

  TacBlendStateData blendStateData;
  blendStateData.srcRGB = TacBlendConstants::One;
  blendStateData.dstRGB = TacBlendConstants::Zero;
  blendStateData.blendRGB = TacBlendMode::Add;
  blendStateData.srcA = TacBlendConstants::Zero;
  blendStateData.dstA = TacBlendConstants::One;
  blendStateData.blendA = TacBlendMode::Add;
  blendStateData.mName = "skybox";
  blendStateData.mStackFrame = TAC_STACK_FRAME;
  TacRenderer::Instance->AddBlendState( &mBlendState, blendStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacDepthStateData depthStateData;
  depthStateData.depthTest = true;
  depthStateData.depthWrite = true;
  depthStateData.depthFunc = TacDepthFunc::LessOrEqual;
  depthStateData.mName = "skybox";
  depthStateData.mStackFrame = TAC_STACK_FRAME;
  TacRenderer::Instance->AddDepthState( &mDepthState, depthStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacRasterizerStateData rasterizerStateData;
  rasterizerStateData.cullMode = TacCullMode::None; // todo
  rasterizerStateData.fillMode = TacFillMode::Solid;
  rasterizerStateData.frontCounterClockwise = true;
  rasterizerStateData.mName = "skybox";
  rasterizerStateData.mStackFrame = TAC_STACK_FRAME;
  rasterizerStateData.multisample = false;
  rasterizerStateData.scissor = true;
  TacRenderer::Instance->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacSamplerStateData samplerStateData;
  samplerStateData.mName = "skybox";
  samplerStateData.mStackFrame = TAC_STACK_FRAME;
  samplerStateData.filter = TacFilter::Linear;
  TacRenderer::Instance->AddSamplerState( &mSamplerState, samplerStateData, errors );
  TAC_HANDLE_ERROR( errors );
}
void TacSkyboxPresentation::RenderSkybox( const TacString& skyboxDir )
{
  TacRenderer* renderer = TacRenderer::Instance;
  static TacString defaultSkybox = "assets/skybox/daylight";
  const TacString& skyboxDirToUse = skyboxDir.empty() ? defaultSkybox : skyboxDir;

  TacTextureAssetManager* textureAssetManager = TacTextureAssetManager::Instance;
  TacTexture* cubemap;
  TacErrors errors;
  textureAssetManager->GetTextureCube( &cubemap, skyboxDirToUse, errors );
  TacAssert( errors.empty() );
  if( !cubemap )
    return;


  TacMesh* mesh;
  TacModelAssetManager::Instance->GetMesh( &mesh, "assets/editor/Box.gltf", mVertexFormat, errors );
  TacAssert( errors.empty() );
  if( !mesh )
    return;

  static int i;
  ++i;

  float a;
  float b;
  TacRenderer::Instance->GetPerspectiveProjectionAB( mCamera->mFarPlane, mCamera->mNearPlane, a, b );
  float aspect = ( float )mDesktopWindow->mWidth / ( float )mDesktopWindow->mHeight;

  TacDefaultCBufferPerFrame perFrame;
  perFrame.mFar = mCamera->mFarPlane;
  perFrame.mNear = mCamera->mNearPlane;
  perFrame.mGbufferSize = { ( float )mDesktopWindow->mWidth, ( float )mDesktopWindow->mHeight };
  perFrame.mView = M4ViewInv( v3( 0, 0, 0 ), mCamera->mForwards, mCamera->mRight, mCamera->mUp );
  perFrame.mProjection = mCamera->Proj( a, b, aspect );

  TacDrawCall2 drawCallPerFrame = {};
  drawCallPerFrame.mUniformDst = mPerFrame;
  drawCallPerFrame.mUniformSrcc = TacTemporaryMemory( perFrame );
  TacRenderer::Instance->AddDrawCall( drawCallPerFrame );

  TacSubMesh* subMesh = &mesh->mSubMeshes[ 0 ];

  TacDrawCall2 drawCallGeometry = {};
  drawCallGeometry.mVertexBuffer = subMesh->mVertexBuffer;
  drawCallGeometry.mIndexBuffer = subMesh->mIndexBuffer;
  drawCallGeometry.mIndexCount = subMesh->mIndexBuffer->indexCount;
  drawCallGeometry.mVertexFormat = mVertexFormat;
  drawCallGeometry.mShader = mShader;
  drawCallGeometry.mBlendState = mBlendState;
  drawCallGeometry.mDepthState = mDepthState;
  drawCallGeometry.mPrimitiveTopology = TacPrimitiveTopology::TriangleList;
  drawCallGeometry.mRasterizerState = mRasterizerState;
  drawCallGeometry.mSamplerState = mSamplerState;
  drawCallGeometry.mStackFrame = TAC_STACK_FRAME;
  drawCallGeometry.mStartIndex = 0;
  drawCallGeometry.mTexture = cubemap;
  drawCallGeometry.mRenderView = mDesktopWindow->mRenderView;
  TacRenderer::Instance->AddDrawCall( drawCallGeometry );

  TacRenderer::Instance->DebugBegin( "Skybox" );
  TacRenderer::Instance->RenderFlush();
  TacRenderer::Instance->DebugEnd();
}
