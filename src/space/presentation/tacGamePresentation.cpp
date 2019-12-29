#include "common/tacCamera.h"
#include "common/tacMemory.h"
#include "common/assetmanagers/tacModelAssetManager.h"
#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/math/tacMatrix4.h"
#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacDebug3D.h"
#include "common/tacDesktopWindow.h"
#include "space/presentation/tacSkyboxPresentation.h"
#include "space/presentation/tacGamePresentation.h"
#include "space/graphics/tacgraphics.h"
#include "space/physics/tacphysics.h"
#include "space/terrain/tacterrain.h"
#include "space/tacworld.h"
#include "space/model/tacmodel.h"
#include "space/tacentity.h"


struct TacTerrainVertex
{
  v3 mPos;
  v2 mUV;
};

TacGamePresentation::~TacGamePresentation()
{
  TacRenderer* renderer = TacRenderer::Instance;
  TacRenderer::Instance->RemoveRendererResource( m3DShader );
  TacRenderer::Instance->RemoveRendererResource( m3DVertexFormat );
  TacRenderer::Instance->RemoveRendererResource( mPerFrame );
  TacRenderer::Instance->RemoveRendererResource( mPerObj );
  TacRenderer::Instance->RemoveRendererResource( mDepthState );
  TacRenderer::Instance->RemoveRendererResource( mBlendState );
  TacRenderer::Instance->RemoveRendererResource( mRasterizerState );
  TacRenderer::Instance->RemoveRendererResource( mSamplerState );
}
void TacGamePresentation::RenderGameWorldToDesktopView()
{
  TacTextureAssetManager* textureAssetManager = TacTextureAssetManager::Instance;
  TacModelAssetManager* modelAssetManager = TacModelAssetManager::Instance;
  TacRenderer* renderer = TacRenderer::Instance;

  TacWorld* world = mWorld;

  m4 view = mCamera->View();
  float a;
  float b;
  TacRenderer::Instance->GetPerspectiveProjectionAB( mCamera->mFarPlane, mCamera->mNearPlane, a, b );

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
  TacRenderer::Instance->AddDrawCall( setPerFrame );


  TacGraphics* graphics = TacGraphics::GetSystem( world );
  TacPhysics* physics = TacPhysics::GetSystem( world );
  for( TacModel* model : graphics->mModels )
  {
    TacMesh* mesh = model->mesh;
    if( !mesh )
    {
      // Try load mesh
      if( model->mGLTFPath.empty() )
        continue;
      TacErrors getmeshErrors;
      TacModelAssetManager::Instance->GetMesh( &mesh, model->mGLTFPath, m3DVertexFormat, getmeshErrors );
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
  for( TacTerrain* terrain : physics->mTerrains )
  {
    if( !terrain->mVertexBuffer || !terrain->mIndexBuffer )
    {
      if( terrain->mRowMajorGrid.empty() )
        continue;

      typedef uint32_t TerrainIndex;

      TacVector< TacTerrainVertex > vertexes;
      TacVector< TerrainIndex > indexes; 

      for( int iRow = 1; iRow < terrain->mSideVertexCount; ++iRow )
      {
        for( int iCol = 1; iCol < terrain->mSideVertexCount; ++iCol )
        {
          TacTerrainVertex vertexTL = {};
          vertexTL.mPos = terrain->GetGridVal( iRow - 1, iCol - 1 );
          vertexTL.mUV = { 0, 1 };
          int iVertexTL = vertexes.size();
          vertexes.push_back(vertexTL);

          TacTerrainVertex vertexTR = {};
          vertexTR.mPos = terrain->GetGridVal( iRow - 1, iCol );
          vertexTR.mUV = { 1, 1 };
          int iVertexTR  = vertexes.size();
          vertexes.push_back(vertexTR);

          TacTerrainVertex vertexBL = {};
          vertexBL.mPos = terrain->GetGridVal( iRow, iCol - 1 );
          vertexBL.mUV = { 0, 0 };
          int iVertexBL = vertexes.size();
          vertexes.push_back(vertexBL);

          TacTerrainVertex vertexBR = {};
          vertexBR.mPos = terrain->GetGridVal( iRow, iCol );
          vertexBR.mUV = { 1, 0 };
          int iVertexBR = vertexes.size();
          vertexes.push_back(vertexBR);

          indexes.push_back( iVertexBR );
          indexes.push_back( iVertexTL );
          indexes.push_back( iVertexBL );

          indexes.push_back( iVertexBR );
          indexes.push_back( iVertexTR );
          indexes.push_back( iVertexTL );
        }
      }
      
      TacErrors rendererResourceErrors;

      TacVertexBufferData vertexBufferData = {};
      vertexBufferData.mAccess = TacAccess::Default;
      vertexBufferData.mNumVertexes = vertexes.size();
      vertexBufferData.mOptionalData = vertexes.data();
      vertexBufferData.mStrideBytesBetweenVertexes = sizeof( TacTerrainVertex );
      vertexBufferData.mName = terrain->mHeightmapTexturePath + "terrain vtx buffer";
      vertexBufferData.mStackFrame = TAC_STACK_FRAME;
      TacRenderer::Instance->AddVertexBuffer( &terrain->mVertexBuffer, vertexBufferData, rendererResourceErrors );
      if( rendererResourceErrors.size() )
        continue;

      TacIndexBufferData indexBufferData = {};
      indexBufferData.mAccess = TacAccess::Default;
      indexBufferData.mData = indexes.data();
      indexBufferData.mFormat.mElementCount = 1;
      indexBufferData.mFormat.mPerElementByteCount = sizeof( TerrainIndex );
      indexBufferData.mFormat.mPerElementDataType = TacGraphicsType::uint;
      indexBufferData.mIndexCount = indexes.size();
      indexBufferData.mName = terrain->mHeightmapTexturePath + "terrain idx buffer";
      indexBufferData.mStackFrame = TAC_STACK_FRAME;
      TacRenderer::Instance->AddIndexBuffer( &terrain->mIndexBuffer, indexBufferData, rendererResourceErrors );
      if( rendererResourceErrors.size() )
        continue;

    }

    if( !terrain->mVertexBuffer || !terrain->mIndexBuffer )
      continue;

    TacTexture* terrainTexture = nullptr;
    TacTexture* noiseTexture = nullptr;
    TacErrors errors;
    textureAssetManager->GetTexture( &terrainTexture, terrain->mGroundTexturePath, errors );
    textureAssetManager->GetTexture( &noiseTexture, terrain->mNoiseTexturePath, errors );

    TacDefaultCBufferPerObject cbuf = {};
    cbuf.Color = { 1, 1, 1, 1 };
    cbuf.World = m4::Identity();

    TacDrawCall2 drawCall = {};
    drawCall.mBlendState = mBlendState;
    drawCall.mDepthState = mDepthState;
    drawCall.mIndexBuffer = terrain->mIndexBuffer;
    drawCall.mIndexCount = terrain->mIndexBuffer->mIndexCount;
    drawCall.mPrimitiveTopology = TacPrimitiveTopology::TriangleList;
    drawCall.mRasterizerState = mRasterizerState;
    drawCall.mRenderView = mDesktopWindow->mRenderView;
    drawCall.mSamplerState = mSamplerState;
    drawCall.mShader = mTerrainShader;
    drawCall.mStackFrame = TAC_STACK_FRAME;
    drawCall.mStartIndex = 0;
    drawCall.mTextures = { terrainTexture, noiseTexture };
    drawCall.mUniformDst = mPerObj;
    drawCall.mUniformSrcc = TacTemporaryMemory( &cbuf, sizeof( TacDefaultCBufferPerObject ) );
    drawCall.mVertexBuffer = terrain->mVertexBuffer;
    drawCall.mVertexCount = terrain->mVertexBuffer->mNumVertexes;
    drawCall.mVertexFormat = mTerrainVertexFormat;

    renderer->AddDrawCall( drawCall );
  }
  TacRenderer::Instance->DebugBegin( "Render game world" );
  TacRenderer::Instance->RenderFlush();
  TacRenderer::Instance->DebugEnd();
  mSkyboxPresentation->RenderSkybox( world->mSkyboxDir );

  TacErrors ignored;
  world->mDebug3DDrawData->DrawToTexture(
    ignored,
    &perFrameData,
    mDesktopWindow->mRenderView );
}
void TacGamePresentation::CreateTerrainShader( TacErrors& errors )
{
  TacShaderData shaderData;
  shaderData.mStackFrame = TAC_STACK_FRAME;
  shaderData.mName = "game window terrain shader";
  shaderData.mShaderPath = "Terrain";
  shaderData.mCBuffers = { mPerFrame, mPerObj };
  TacRenderer::Instance->AddShader( &mTerrainShader, shaderData, errors );
}
void TacGamePresentation::Create3DShader( TacErrors& errors )
{
  TacShaderData shaderData;
  shaderData.mStackFrame = TAC_STACK_FRAME;
  shaderData.mName = "game window 3d shader";
  shaderData.mShaderPath = "3DTest";
  shaderData.mCBuffers = { mPerFrame, mPerObj };
  TacRenderer::Instance->AddShader( &m3DShader, shaderData, errors );
}
void TacGamePresentation::Create3DVertexFormat( TacErrors& errors )
{
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
  TacRenderer::Instance->AddVertexFormat( &m3DVertexFormat, vertexFormatData, errors );
}
void TacGamePresentation::CreateTerrainVertexFormat( TacErrors& errors )
{
  TacVertexDeclaration terrainPosDecl = {};
  terrainPosDecl.mAttribute = TacAttribute::Position;
  terrainPosDecl.mTextureFormat.mElementCount = 3;
  terrainPosDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
  terrainPosDecl.mTextureFormat.mPerElementDataType = TacGraphicsType::real;
  terrainPosDecl.mAlignedByteOffset = TacOffsetOf( TacTerrainVertex, mPos );

  TacVertexDeclaration terrainTexCoordDecl = {};
  terrainTexCoordDecl.mAttribute = TacAttribute::Texcoord;
  terrainTexCoordDecl.mTextureFormat.mElementCount = 2;
  terrainTexCoordDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
  terrainTexCoordDecl.mTextureFormat.mPerElementDataType = TacGraphicsType::real;
  terrainTexCoordDecl.mAlignedByteOffset = TacOffsetOf( TacTerrainVertex, mUV );

  TacVertexFormatData vertexFormatData = {};
  vertexFormatData.shader = mTerrainShader;
  vertexFormatData.vertexFormatDatas =
  {
    terrainPosDecl,
    terrainTexCoordDecl
  };
  vertexFormatData.mStackFrame = TAC_STACK_FRAME;
  vertexFormatData.mName = "terrain vertex format";
  TacRenderer::Instance->AddVertexFormat( &mTerrainVertexFormat, vertexFormatData, errors );
}
void TacGamePresentation::CreatePerFrame( TacErrors& errors )
{
  TacCBufferData cBufferDataPerFrame = {};
  cBufferDataPerFrame.mName = "tac 3d per frame";
  cBufferDataPerFrame.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerFrame.shaderRegister = 0;
  cBufferDataPerFrame.byteCount = sizeof( TacDefaultCBufferPerFrame );
  TacRenderer::Instance->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
}
void TacGamePresentation::CreatePerObj( TacErrors& errors )
{
  TacCBufferData cBufferDataPerObj = {};
  cBufferDataPerObj.mName = "tac 3d per obj";
  cBufferDataPerObj.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerObj.shaderRegister = 1;
  cBufferDataPerObj.byteCount = sizeof( TacDefaultCBufferPerObject );
  TacRenderer::Instance->AddConstantBuffer( &mPerObj, cBufferDataPerObj, errors );
}
void TacGamePresentation::CreateDepthState( TacErrors& errors )
{
  TacDepthStateData depthStateData;
  depthStateData.depthTest = true;
  depthStateData.depthWrite = true;
  depthStateData.depthFunc = TacDepthFunc::Less;
  depthStateData.mName = "tac 3d depth state";
  depthStateData.mStackFrame = TAC_STACK_FRAME;
  TacRenderer::Instance->AddDepthState( &mDepthState, depthStateData, errors );
}
void TacGamePresentation::CreateBlendState( TacErrors& errors )
{
  TacBlendStateData blendStateData;
  blendStateData.srcRGB = TacBlendConstants::One;
  blendStateData.dstRGB = TacBlendConstants::Zero;
  blendStateData.blendRGB = TacBlendMode::Add;
  blendStateData.srcA = TacBlendConstants::Zero;
  blendStateData.dstA = TacBlendConstants::One;
  blendStateData.blendA = TacBlendMode::Add;
  blendStateData.mName = "tac 3d opaque blend";
  blendStateData.mStackFrame = TAC_STACK_FRAME;
  TacRenderer::Instance->AddBlendState( &mBlendState, blendStateData, errors );
}
void TacGamePresentation::CreateRasterizerState( TacErrors& errors )
{
  TacRasterizerStateData rasterizerStateData;
  rasterizerStateData.cullMode = TacCullMode::None; // todo
  rasterizerStateData.fillMode = TacFillMode::Solid;
  rasterizerStateData.frontCounterClockwise = true;
  rasterizerStateData.mName = "tac 3d rast state";
  rasterizerStateData.mStackFrame = TAC_STACK_FRAME;
  rasterizerStateData.multisample = false;
  rasterizerStateData.scissor = true;
  TacRenderer::Instance->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
}
void TacGamePresentation::CreateSamplerState( TacErrors& errors )
{
  TacSamplerStateData samplerStateData;
  samplerStateData.mName = "tac 3d tex sampler";
  samplerStateData.mStackFrame = TAC_STACK_FRAME;
  samplerStateData.filter = TacFilter::Linear;
  TacRenderer::Instance->AddSamplerState( &mSamplerState, samplerStateData, errors );
}
void TacGamePresentation::CreateGraphicsObjects( TacErrors& errors )
{
  CreatePerFrame( errors );
  TAC_HANDLE_ERROR( errors );

  CreatePerObj( errors );
  TAC_HANDLE_ERROR( errors );

  Create3DShader( errors );
  TAC_HANDLE_ERROR( errors );

  CreateTerrainShader( errors );
  TAC_HANDLE_ERROR( errors );

  Create3DVertexFormat( errors );
  TAC_HANDLE_ERROR( errors );

  CreateTerrainVertexFormat( errors );
  TAC_HANDLE_ERROR( errors );

  CreateBlendState( errors );
  TAC_HANDLE_ERROR( errors );

  CreateDepthState( errors );
  TAC_HANDLE_ERROR( errors );

  CreateRasterizerState( errors );
  TAC_HANDLE_ERROR( errors );

  CreateSamplerState( errors );
  TAC_HANDLE_ERROR( errors );
}
void TacGamePresentation::RenderGameWorldAddDrawCall(
  const TacMesh* mesh,
  const TacDefaultCBufferPerObject& cbuf )
{
  TacRenderer* renderer = TacRenderer::Instance;
  for( const TacSubMesh& subMesh : mesh->mSubMeshes )
  {
    TacDrawCall2 drawCall = {};
    drawCall.mShader = mesh->mVertexFormat->shader;
    drawCall.mVertexBuffer = subMesh.mVertexBuffer;
    drawCall.mIndexBuffer = subMesh.mIndexBuffer;
    drawCall.mStartIndex = 0;
    drawCall.mIndexCount = subMesh.mIndexBuffer->mIndexCount;
    drawCall.mRenderView = mDesktopWindow->mRenderView;
    drawCall.mBlendState = mBlendState;
    drawCall.mRasterizerState = mRasterizerState;
    drawCall.mSamplerState = mSamplerState;
    drawCall.mDepthState = mDepthState;
    drawCall.mVertexFormat = mesh->mVertexFormat;
    drawCall.mUniformDst = mPerObj;
    drawCall.mUniformSrcc = TacTemporaryMemory( &cbuf, sizeof( TacDefaultCBufferPerObject ) );
    drawCall.mStackFrame = TAC_STACK_FRAME;
    TacRenderer::Instance->AddDrawCall( drawCall );
  }
}
