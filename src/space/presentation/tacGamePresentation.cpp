#include "common/tacCamera.h"
#include "common/tacMemory.h"
#include "common/assetmanagers/tacModelAssetManager.h"
#include "common/math/tacMatrix4.h"
#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacDebug3D.h"
#include "common/tacDesktopWindow.h"
#include "space/presentation/tacSkyboxPresentation.h"
#include "space/presentation/tacGamePresentation.h"
#include "space/tacgraphics.h"
#include "space/tacworld.h"
#include "space/tacmodel.h"
#include "space/tacentity.h"

TacGamePresentation::~TacGamePresentation()
{
  TacRenderer* renderer = mRenderer;
  renderer->RemoveRendererResource( m3DShader );
  renderer->RemoveRendererResource( m3DVertexFormat );
  renderer->RemoveRendererResource( mPerFrame );
  renderer->RemoveRendererResource( mPerObj );
  renderer->RemoveRendererResource( mDepthState );
  renderer->RemoveRendererResource( mBlendState );
  renderer->RemoveRendererResource( mRasterizerState );
  renderer->RemoveRendererResource( mSamplerState );
}
void TacGamePresentation::RenderGameWorldToDesktopView()
{
  TacRenderer* renderer = mRenderer;
  TacModelAssetManager* modelAssetManager = mModelAssetManager;
  TacWorld* world = mWorld;

  m4 view = mCamera->View();
  float a;
  float b;
  renderer->GetPerspectiveProjectionAB( mCamera->mFarPlane, mCamera->mNearPlane, a, b );

  float w = ( float )mDesktopWindow->mWidth;
  float h = ( float )mDesktopWindow->mHeight;
  float aspect = w / h;
  m4 proj = mCamera->Proj( a, b, aspect );

  TacDefaultCBufferPerFrame perFrameData;
  perFrameData.mFar = mCamera->mFarPlane;
  perFrameData.mNear = mCamera->mNearPlane;
  perFrameData.mView = view;
  perFrameData.mProjection = proj;
  perFrameData.mGbufferSize = { w, h };
  TacDrawCall2 setPerFrame = {};
  setPerFrame.mUniformDst = mPerFrame;
  setPerFrame.mUniformSrcc = TacTemporaryMemory( &perFrameData, sizeof( TacDefaultCBufferPerFrame ) );
  renderer->AddDrawCall( setPerFrame );


  TacGraphics* graphics = TacGraphics::GetSystem( world );
  for( TacModel* model : graphics->mModels )
  {
    TacMesh* mesh = model->mesh;
    if( !mesh )
    {
      // Try load mesh
      if( model->mGLTFPath.empty() )
        continue;
      TacErrors getmeshErrors;
      modelAssetManager->GetMesh( &mesh, model->mGLTFPath, m3DVertexFormat, getmeshErrors );
      if( getmeshErrors.empty() )
        model->mesh = mesh;
      else
        continue;
    }

    TacEntity* entity = model->mEntity;

    TacDefaultCBufferPerObject perObjectData;
    perObjectData.Color = { model->mColorRGB, 1 }; // { 0.23f, 0.7f, 0.5f, 1 };
    perObjectData.World = entity->mWorldTransform;
    RenderGameWorldAddDrawCall( mesh, perObjectData );
  }
  renderer->DebugBegin( "Render game world" );
  renderer->RenderFlush();
  renderer->DebugEnd();
  mSkyboxPresentation->RenderSkybox( world->mSkyboxDir );

  TacErrors ignored;
  world->mDebug3DDrawData->DrawToTexture(
    ignored,
    &perFrameData,
    mDebug3DCommonData,
    mDesktopWindow->mRenderView );
}
void TacGamePresentation::CreateGraphicsObjects( TacErrors& errors )
{
  TacRenderer* renderer = mRenderer;

  TacCBufferData cBufferDataPerFrame = {};
  cBufferDataPerFrame.mName = "tac 3d per frame";
  cBufferDataPerFrame.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerFrame.shaderRegister = 0;
  cBufferDataPerFrame.byteCount = sizeof( TacDefaultCBufferPerFrame );
  renderer->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
  TAC_HANDLE_ERROR( errors );

  TacCBufferData cBufferDataPerObj = {};
  cBufferDataPerObj.mName = "tac 3d per obj";
  cBufferDataPerObj.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerObj.shaderRegister = 1;
  cBufferDataPerObj.byteCount = sizeof( TacDefaultCBufferPerObject );
  renderer->AddConstantBuffer( &mPerObj, cBufferDataPerObj, errors );
  TAC_HANDLE_ERROR( errors );


  TacShaderData shaderData;
  shaderData.mStackFrame = TAC_STACK_FRAME;
  shaderData.mName = "game window 3d shader";
  shaderData.mShaderPath = "3DTest";
  shaderData.mCBuffers = { mPerFrame, mPerObj };
  renderer->AddShader( &m3DShader, shaderData, errors );
  TAC_HANDLE_ERROR( errors );

  TacVertexDeclaration posDecl;
  posDecl.mAlignedByteOffset = 0;
  posDecl.mAttribute = TacAttribute::Position;
  posDecl.mTextureFormat.mElementCount = 3;
  posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
  posDecl.mTextureFormat.mPerElementDataType = TacGraphicsType::real;
  TacVertexFormatData vertexFormatData = {};
  vertexFormatData.shader = m3DShader;
  vertexFormatData.vertexFormatDatas = { posDecl };
  vertexFormatData.mStackFrame = TAC_STACK_FRAME;
  vertexFormatData.mName = "game window renderer"; // cpresentation?
  renderer->AddVertexFormat( &m3DVertexFormat, vertexFormatData, errors );
  TAC_HANDLE_ERROR( errors );

  TacBlendStateData blendStateData;
  blendStateData.srcRGB = TacBlendConstants::One;
  blendStateData.dstRGB = TacBlendConstants::Zero;
  blendStateData.blendRGB = TacBlendMode::Add;
  blendStateData.srcA = TacBlendConstants::Zero;
  blendStateData.dstA = TacBlendConstants::One;
  blendStateData.blendA = TacBlendMode::Add;
  blendStateData.mName = "tac 3d opaque blend";
  blendStateData.mStackFrame = TAC_STACK_FRAME;
  renderer->AddBlendState( &mBlendState, blendStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacDepthStateData depthStateData;
  depthStateData.depthTest = true;
  depthStateData.depthWrite = true;
  depthStateData.depthFunc = TacDepthFunc::Less;
  depthStateData.mName = "tac 3d depth state";
  depthStateData.mStackFrame = TAC_STACK_FRAME;
  renderer->AddDepthState( &mDepthState, depthStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacRasterizerStateData rasterizerStateData;
  rasterizerStateData.cullMode = TacCullMode::None; // todo
  rasterizerStateData.fillMode = TacFillMode::Solid;
  rasterizerStateData.frontCounterClockwise = true;
  rasterizerStateData.mName = "tac 3d rast state";
  rasterizerStateData.mStackFrame = TAC_STACK_FRAME;
  rasterizerStateData.multisample = false;
  rasterizerStateData.scissor = true;
  renderer->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacSamplerStateData samplerStateData;
  samplerStateData.mName = "tac 3d tex sampler";
  samplerStateData.mStackFrame = TAC_STACK_FRAME;
  samplerStateData.filter = TacFilter::Linear;
  renderer->AddSamplerState( &mSamplerState, samplerStateData, errors );
  TAC_HANDLE_ERROR( errors );
}
void TacGamePresentation::RenderGameWorldAddDrawCall(
  const TacMesh* mesh,
  const TacDefaultCBufferPerObject& cbuf )
{
  TacRenderer* renderer = mRenderer;
  for( const TacSubMesh& subMesh : mesh->mSubMeshes )
  {
    TacDrawCall2 drawCall = {};
    drawCall.mShader = mesh->mVertexFormat->shader;
    drawCall.mVertexBuffer = subMesh.mVertexBuffer;
    drawCall.mIndexBuffer = subMesh.mIndexBuffer;
    drawCall.mStartIndex = 0;
    drawCall.mIndexCount = subMesh.mIndexBuffer->indexCount;
    drawCall.mRenderView = mDesktopWindow->mRenderView;
    drawCall.mBlendState = mBlendState;
    drawCall.mRasterizerState = mRasterizerState;
    drawCall.mSamplerState = mSamplerState;
    drawCall.mDepthState = mDepthState;
    drawCall.mVertexFormat = mesh->mVertexFormat;
    drawCall.mTexture = nullptr;
    drawCall.mUniformDst = mPerObj;
    drawCall.mUniformSrcc = TacTemporaryMemory( &cbuf, sizeof( TacDefaultCBufferPerObject ) );
    drawCall.mStackFrame = TAC_STACK_FRAME;
    renderer->AddDrawCall( drawCall );
  }
}
