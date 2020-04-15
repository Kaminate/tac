#include "src/common/tacCamera.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacMemory.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/tacDesktopWindow.h"
#include "src/space/presentation/tacSkyboxPresentation.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/physics/tacPhysics.h"
#include "src/space/terrain/tacTerrain.h"
#include "src/space/tacWorld.h"
#include "src/space/model/tacModel.h"
#include "src/space/tacEntity.h"

namespace Tac
{


  struct TerrainVertex
  {
    v3 mPos;
    v2 mUV;
  };

  GamePresentation::~GamePresentation()
  {
    Renderer::Instance->RemoveRendererResource( m3DShader );
    Renderer::Instance->RemoveRendererResource( m3DVertexFormat );
    Renderer::Instance->RemoveRendererResource( mPerFrame );
    Renderer::Instance->RemoveRendererResource( mPerObj );
    Renderer::Instance->RemoveRendererResource( mDepthState );
    Renderer::Instance->RemoveRendererResource( mBlendState );
    Renderer::Instance->RemoveRendererResource( mRasterizerState );
    Renderer::Instance->RemoveRendererResource( mSamplerState );
  }
  void GamePresentation::RenderGameWorldToDesktopView()
  {
    //_PROFILE_BLOCK;

    World* world = mWorld;

    m4 view = mCamera->View();
    float a;
    float b;
    Renderer::Instance->GetPerspectiveProjectionAB( mCamera->mFarPlane, mCamera->mNearPlane, a, b );

    float w = ( float )mDesktopWindow->mWidth;
    float h = ( float )mDesktopWindow->mHeight;
    float aspect = w / h;
    m4 proj = mCamera->Proj( a, b, aspect );

    DefaultCBufferPerFrame perFrameData;
    perFrameData.mFar = mCamera->mFarPlane;
    perFrameData.mNear = mCamera->mNearPlane;
    perFrameData.mView = view;
    perFrameData.mProjection = proj;
    perFrameData.mGbufferSize = { w, h };
    DrawCall2 setPerFrame = {};
    setPerFrame.mUniformDst = mPerFrame;
    setPerFrame.CopyUniformSource( perFrameData );
    Renderer::Instance->AddDrawCall( setPerFrame );


    Graphics* graphics = Graphics::GetSystem( world );
    Physics* physics = Physics::GetSystem( world );
    for( Model* model : graphics->mModels )
    {
      Mesh* mesh = model->mesh;
      if( !mesh )
      {
        // Try load mesh
        if( model->mGLTFPath.empty() )
          continue;
        Errors getmeshErrors;
        ModelAssetManager::Instance->GetMesh( &mesh, model->mGLTFPath, m3DVertexFormat, getmeshErrors );
        if( getmeshErrors.empty() )
          model->mesh = mesh;
        else
          continue;
      }

      Entity* entity = model->mEntity;

      DefaultCBufferPerObject perObjectData;
      perObjectData.Color = { model->mColorRGB, 1 }; // { 0.23f, 0.7f, 0.5f, 1 };
      perObjectData.World = entity->mWorldTransform;
      RenderGameWorldAddDrawCall( mesh, perObjectData );
    }
    for( Terrain* terrain : physics->mTerrains )
    {
      if( !terrain->mVertexBuffer || !terrain->mIndexBuffer )
      {
        if( terrain->mRowMajorGrid.empty() )
          continue;

        typedef uint32_t TerrainIndex;

        Vector< TerrainVertex > vertexes;
        Vector< TerrainIndex > indexes;

        for( int iRow = 1; iRow < terrain->mSideVertexCount; ++iRow )
        {
          for( int iCol = 1; iCol < terrain->mSideVertexCount; ++iCol )
          {
            TerrainVertex vertexTL = {};
            vertexTL.mPos = terrain->GetGridVal( iRow - 1, iCol - 1 );
            vertexTL.mUV = { 0, 1 };
            int iVertexTL = vertexes.size();
            vertexes.push_back( vertexTL );

            TerrainVertex vertexTR = {};
            vertexTR.mPos = terrain->GetGridVal( iRow - 1, iCol );
            vertexTR.mUV = { 1, 1 };
            int iVertexTR = vertexes.size();
            vertexes.push_back( vertexTR );

            TerrainVertex vertexBL = {};
            vertexBL.mPos = terrain->GetGridVal( iRow, iCol - 1 );
            vertexBL.mUV = { 0, 0 };
            int iVertexBL = vertexes.size();
            vertexes.push_back( vertexBL );

            TerrainVertex vertexBR = {};
            vertexBR.mPos = terrain->GetGridVal( iRow, iCol );
            vertexBR.mUV = { 1, 0 };
            int iVertexBR = vertexes.size();
            vertexes.push_back( vertexBR );

            indexes.push_back( iVertexBR );
            indexes.push_back( iVertexTL );
            indexes.push_back( iVertexBL );

            indexes.push_back( iVertexBR );
            indexes.push_back( iVertexTR );
            indexes.push_back( iVertexTL );
          }
        }

        Errors rendererResourceErrors;

        VertexBufferData vertexBufferData = {};
        vertexBufferData.mAccess = Access::Default;
        vertexBufferData.mNumVertexes = vertexes.size();
        vertexBufferData.mOptionalData = vertexes.data();
        vertexBufferData.mStrideBytesBetweenVertexes = sizeof( TerrainVertex );
        vertexBufferData.mName = terrain->mHeightmapTexturePath + "terrain vtx buffer";
        vertexBufferData.mFrame = TAC_STACK_FRAME;
        Renderer::Instance->AddVertexBuffer( &terrain->mVertexBuffer, vertexBufferData, rendererResourceErrors );
        if( rendererResourceErrors )
          continue;

        IndexBufferData indexBufferData = {};
        indexBufferData.mAccess = Access::Default;
        indexBufferData.mData = indexes.data();
        indexBufferData.mFormat.mElementCount = 1;
        indexBufferData.mFormat.mPerElementByteCount = sizeof( TerrainIndex );
        indexBufferData.mFormat.mPerElementDataType = GraphicsType::uint;
        indexBufferData.mIndexCount = indexes.size();
        indexBufferData.mName = terrain->mHeightmapTexturePath + "terrain idx buffer";
        indexBufferData.mFrame = TAC_STACK_FRAME;
        Renderer::Instance->AddIndexBuffer( &terrain->mIndexBuffer, indexBufferData, rendererResourceErrors );
        if( rendererResourceErrors )
          continue;

      }

      if( !terrain->mVertexBuffer || !terrain->mIndexBuffer )
        continue;

      Errors errors;
      Render::TextureHandle terrainTexture =
        TextureAssetManager::GetTexture( terrain->mGroundTexturePath, errors );
      Render::TextureHandle noiseTexture =
        TextureAssetManager::GetTexture( terrain->mNoiseTexturePath, errors );

      DefaultCBufferPerObject cbuf = {};
      cbuf.Color = { 1, 1, 1, 1 };
      cbuf.World = m4::Identity();

      DrawCall2 drawCall = {};
      drawCall.mBlendState = mBlendState;
      drawCall.mDepthState = mDepthState;
      drawCall.mIndexBuffer = terrain->mIndexBuffer;
      drawCall.mIndexCount = terrain->mIndexBuffer->mIndexCount;
      drawCall.mPrimitiveTopology = PrimitiveTopology::TriangleList;
      drawCall.mRasterizerState = mRasterizerState;
      drawCall.mRenderView = mDesktopWindow->mRenderView;
      drawCall.mSamplerState = mSamplerState;
      drawCall.mShader = mTerrainShader;
      drawCall.mFrame = TAC_STACK_FRAME;
      drawCall.mStartIndex = 0;
      drawCall.mTextures = { nullptr, nullptr }; // { terrainTexture, noiseTexture };
      drawCall.mUniformDst = mPerObj;
      drawCall.mUniformSrcc = TemporaryMemoryFromT( cbuf );
      drawCall.mVertexBuffer = terrain->mVertexBuffer;
      drawCall.mVertexCount = terrain->mVertexBuffer->mNumVertexes;
      drawCall.mVertexFormat = mTerrainVertexFormat;

      Renderer::Instance->AddDrawCall( drawCall );
    }
    Renderer::Instance->DebugBegin( "Render game world" );
    Renderer::Instance->RenderFlush();
    Renderer::Instance->DebugEnd();
    mSkyboxPresentation->RenderSkybox( world->mSkyboxDir );

    Errors ignored;
    world->mDebug3DDrawData->DrawToTexture(
      ignored,
      &perFrameData,
      mDesktopWindow->mRenderView );
  }
  void GamePresentation::CreateTerrainShader( Errors& errors )
  {
    ShaderData shaderData;
    shaderData.mFrame = TAC_STACK_FRAME;
    shaderData.mName = "game window terrain shader";
    shaderData.mShaderPath = "Terrain";
    shaderData.mCBuffers = { mPerFrame, mPerObj };
    Renderer::Instance->AddShader( &mTerrainShader, shaderData, errors );
  }
  void GamePresentation::Create3DShader( Errors& errors )
  {
    ShaderData shaderData;
    shaderData.mFrame = TAC_STACK_FRAME;
    shaderData.mName = "game window 3d shader";
    shaderData.mShaderPath = "3DTest";
    shaderData.mCBuffers = { mPerFrame, mPerObj };
    Renderer::Instance->AddShader( &m3DShader, shaderData, errors );
  }
  void GamePresentation::Create3DVertexFormat( Errors& errors )
  {
    VertexDeclaration posDecl;
    posDecl.mAlignedByteOffset = 0;
    posDecl.mAttribute = Attribute::Position;
    posDecl.mTextureFormat.mElementCount = 3;
    posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    posDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
    VertexFormatData vertexFormatData = {};
    vertexFormatData.shader = m3DShader;
    vertexFormatData.vertexFormatDatas = { posDecl };
    vertexFormatData.mFrame = TAC_STACK_FRAME;
    vertexFormatData.mName = "game window renderer"; // cpresentation?
    Renderer::Instance->AddVertexFormat( &m3DVertexFormat, vertexFormatData, errors );
  }
  void GamePresentation::CreateTerrainVertexFormat( Errors& errors )
  {
    VertexDeclaration terrainPosDecl = {};
    terrainPosDecl.mAttribute = Attribute::Position;
    terrainPosDecl.mTextureFormat.mElementCount = 3;
    terrainPosDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainPosDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
    terrainPosDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mPos );

    VertexDeclaration terrainTexCoordDecl = {};
    terrainTexCoordDecl.mAttribute = Attribute::Texcoord;
    terrainTexCoordDecl.mTextureFormat.mElementCount = 2;
    terrainTexCoordDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainTexCoordDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
    terrainTexCoordDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mUV );

    VertexFormatData vertexFormatData = {};
    vertexFormatData.shader = mTerrainShader;
    vertexFormatData.vertexFormatDatas =
    {
      terrainPosDecl,
      terrainTexCoordDecl
    };
    vertexFormatData.mFrame = TAC_STACK_FRAME;
    vertexFormatData.mName = "terrain vertex format";
    Renderer::Instance->AddVertexFormat( &mTerrainVertexFormat, vertexFormatData, errors );
  }
  void GamePresentation::CreatePerFrame( Errors& errors )
  {
    CBufferData cBufferDataPerFrame = {};
    cBufferDataPerFrame.mName = "tac 3d per frame";
    cBufferDataPerFrame.mFrame = TAC_STACK_FRAME;
    cBufferDataPerFrame.shaderRegister = 0;
    cBufferDataPerFrame.byteCount = sizeof( DefaultCBufferPerFrame );
    Renderer::Instance->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
  }
  void GamePresentation::CreatePerObj( Errors& errors )
  {
    CBufferData cBufferDataPerObj = {};
    cBufferDataPerObj.mName = "tac 3d per obj";
    cBufferDataPerObj.mFrame = TAC_STACK_FRAME;
    cBufferDataPerObj.shaderRegister = 1;
    cBufferDataPerObj.byteCount = sizeof( DefaultCBufferPerObject );
    Renderer::Instance->AddConstantBuffer( &mPerObj, cBufferDataPerObj, errors );
  }
  void GamePresentation::CreateDepthState( Errors& errors )
  {
    DepthStateData depthStateData;
    depthStateData.depthTest = true;
    depthStateData.depthWrite = true;
    depthStateData.depthFunc = DepthFunc::Less;
    depthStateData.mName = "tac 3d depth state";
    depthStateData.mFrame = TAC_STACK_FRAME;
    Renderer::Instance->AddDepthState( &mDepthState, depthStateData, errors );
  }
  void GamePresentation::CreateBlendState( Errors& errors )
  {
    BlendStateData blendStateData;
    blendStateData.srcRGB = BlendConstants::One;
    blendStateData.dstRGB = BlendConstants::Zero;
    blendStateData.blendRGB = BlendMode::Add;
    blendStateData.srcA = BlendConstants::Zero;
    blendStateData.dstA = BlendConstants::One;
    blendStateData.blendA = BlendMode::Add;
    blendStateData.mName = "tac 3d opaque blend";
    blendStateData.mFrame = TAC_STACK_FRAME;
    Renderer::Instance->AddBlendState( &mBlendState, blendStateData, errors );
  }
  void GamePresentation::CreateRasterizerState( Errors& errors )
  {
    RasterizerStateData rasterizerStateData;
    rasterizerStateData.cullMode = CullMode::None; // todo
    rasterizerStateData.fillMode = FillMode::Solid;
    rasterizerStateData.frontCounterClockwise = true;
    rasterizerStateData.mName = "tac 3d rast state";
    rasterizerStateData.mFrame = TAC_STACK_FRAME;
    rasterizerStateData.multisample = false;
    rasterizerStateData.scissor = true;
    Renderer::Instance->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
  }
  void GamePresentation::CreateSamplerState( Errors& errors )
  {
    SamplerStateData samplerStateData;
    samplerStateData.mName = "tac 3d tex sampler";
    samplerStateData.mFrame = TAC_STACK_FRAME;
    samplerStateData.filter = Filter::Aniso;
    Renderer::Instance->AddSamplerState( &mSamplerState, samplerStateData, errors );
  }
  void GamePresentation::CreateGraphicsObjects( Errors& errors )
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
  void GamePresentation::RenderGameWorldAddDrawCall(
    const Mesh* mesh,
    const DefaultCBufferPerObject& cbuf )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      DrawCall2 drawCall = {};
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
      drawCall.mUniformSrcc = TemporaryMemoryFromT( cbuf );
      drawCall.mFrame = TAC_STACK_FRAME;
      Renderer::Instance->AddDrawCall( drawCall );
    }
  }
}
