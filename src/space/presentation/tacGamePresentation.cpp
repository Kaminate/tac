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
    Render::DestroyShader( m3DShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( m3DVertexFormat, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mPerFrame, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mPerObj, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
    Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerState, TAC_STACK_FRAME );
  }
  void GamePresentation::RenderGameWorldToDesktopView( const int viewWidth,
                                                       const int viewHeight,
                                                       const Render::ViewId viewId )
  {
    //_PROFILE_BLOCK;

    World* world = mWorld;

    float a;
    float b;
    Render::GetPerspectiveProjectionAB( mCamera->mFarPlane,
                                        mCamera->mNearPlane,
                                        a,
                                        b );

    const float w = ( float )viewWidth;
    const float h = ( float )viewHeight;
    const float aspect = w / h;
    const m4 view = mCamera->View();
    const m4 proj = mCamera->Proj( a, b, aspect );

    DefaultCBufferPerFrame perFrameData;
    perFrameData.mFar = mCamera->mFarPlane;
    perFrameData.mNear = mCamera->mNearPlane;
    perFrameData.mView = view;
    perFrameData.mProjection = proj;
    perFrameData.mGbufferSize = { w, h };
    DrawCall2 setPerFrame = {};
    setPerFrame.mUniformDst = mPerFrame;
    setPerFrame.CopyUniformSource( perFrameData );
    Render::AddDrawCall( setPerFrame );

    Render::UpdateConstantBuffer( mPerFrame,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );

    Render::Submit( viewId );


    Graphics* graphics = Graphics::GetSystem( world );
    for( Model* model : graphics->mModels )
    {
      Mesh* mesh = model->mesh;
      if( !mesh )
      {
        // Try load mesh
        if( model->mGLTFPath.empty() )
          continue;
        Errors getmeshErrors;
        ModelAssetManager::Instance->GetMesh( &mesh,
                                              model->mGLTFPath,
                                              m3DVertexFormat,
                                              m3DVertexFormatDecls,
                                              k3DVertexFormatDeclCount,
                                              getmeshErrors );
        if( getmeshErrors.empty() )
          model->mesh = mesh;
        else
          continue;
      }

      Entity* entity = model->mEntity;

      DefaultCBufferPerObject perObjectData;
      perObjectData.Color = { model->mColorRGB, 1 }; // { 0.23f, 0.7f, 0.5f, 1 };
      perObjectData.World = entity->mWorldTransform;
      RenderGameWorldAddDrawCall( mesh, perObjectData, viewId );
    }

    Physics* physics = Physics::GetSystem( world );
    for( Terrain* terrain : physics->mTerrains )
    {
      if( !terrain->mVertexBuffer.IsValid() || !terrain->mIndexBuffer.IsValid() )
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

        String vertexBufferName = terrain->mHeightmapTexturePath + "terrain vtx buffer";
        terrain->mVertexBuffer = Render::CreateVertexBuffer( vertexBufferName,
                                                             vertexes.size() * sizeof( TerrainVertex ),
                                                             vertexes.data(),
                                                             sizeof( TerrainVertex ),
                                                             Access::Default,
                                                             TAC_STACK_FRAME );

        Format format;
        format.mElementCount = 1;
        format.mPerElementByteCount = sizeof( TerrainIndex );
        format.mPerElementDataType = GraphicsType::uint;
        String indexBufferName = terrain->mHeightmapTexturePath + "terrain idx buffer";
        terrain->mIndexBuffer = Render::CreateIndexBuffer( indexBufferName,
                                                           indexes.size() * sizeof( TerrainIndex ),
                                                           indexes.data(),
                                                           Access::Default,
                                                           format,
                                                           TAC_STACK_FRAME );

      }

      if( !terrain->mVertexBuffer.IsValid() || !terrain->mIndexBuffer.IsValid() )
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
      drawCall.mIndexCount = terrain->mIndexCapacity;
      drawCall.mPrimitiveTopology = PrimitiveTopology::TriangleList;
      drawCall.mRasterizerState = mRasterizerState;
      //drawCall.mRenderView = mDesktopWindow->mRenderView;
      drawCall.mSamplerState = mSamplerState;
      drawCall.mShader = mTerrainShader;
      drawCall.mFrame = TAC_STACK_FRAME;
      drawCall.mStartIndex = 0;
      drawCall.mTextureHandles = { terrainTexture, noiseTexture };
      drawCall.mUniformDst = mPerObj;
      drawCall.mUniformSrcc = TemporaryMemoryFromT( cbuf );
      drawCall.mVertexBuffer = terrain->mVertexBuffer;
      drawCall.mVertexCount = terrain->mVertexCapacity;
      drawCall.mVertexFormat = mTerrainVertexFormat;

      Render::AddDrawCall( drawCall );
    }

    //Renderer::Instance->DebugBegin( "Render game world" );
    //Renderer::Instance->RenderFlush();
    //Renderer::Instance->DebugEnd();
    mSkyboxPresentation->RenderSkybox( viewWidth, viewHeight, viewId, world->mSkyboxDir );

    Errors ignored;
    //world->mDebug3DDrawData->DrawToTexture(
    //  ignored,
    //  &perFrameData,
    //  mDesktopWindow->mRenderView );
  }
  void GamePresentation::CreateTerrainShader( Errors& errors )
  {
    mTerrainShader = Render::CreateShader( "game window terrain shader",
                                           Render::ShaderSource::FromPath( "Terrain" ),
                                           Render::ConstantBuffers( mPerFrame, mPerObj ),
                                           TAC_STACK_FRAME );
  }
  void GamePresentation::Create3DShader( Errors& errors )
  {
    m3DShader = Render::CreateShader( "game window 3d shader",
                                      Render::ShaderSource::FromPath( "3DTest" ),
                                      Render::ConstantBuffers( mPerFrame, mPerObj ),
                                      TAC_STACK_FRAME );
  }
  void GamePresentation::Create3DVertexFormat( Errors& errors )
  {
    VertexDeclaration posDecl;
    posDecl.mAlignedByteOffset = 0;
    posDecl.mAttribute = Attribute::Position;
    posDecl.mTextureFormat.mElementCount = 3;
    posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    posDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
    Render::VertexDeclarations vertexDeclarations;
    vertexDeclarations.AddVertexDeclaration( posDecl );
    m3DVertexFormat = Render::CreateVertexFormat( "game window renderer",
                                                  vertexDeclarations,
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
    m3DVertexFormatDecls[ 0 ] = posDecl;
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

    mTerrainVertexFormat = Render::CreateVertexFormat( "terrain vertex format",
                                                       Render::VertexDeclarations( terrainPosDecl,
                                                                                   terrainTexCoordDecl ),
                                                       mTerrainShader,
                                                       TAC_STACK_FRAME );
  }
  void GamePresentation::CreatePerFrame( Errors& errors )
  {
    mPerFrame = Render::CreateConstantBuffer( "tac 3d per frame",
                                              sizeof( DefaultCBufferPerFrame ),
                                              0,
                                              TAC_STACK_FRAME );
  }
  void GamePresentation::CreatePerObj( Errors& errors )
  {
    mPerObj = Render::CreateConstantBuffer( "tac 3d per obj",
                                            sizeof( DefaultCBufferPerObject ),
                                            1,
                                            TAC_STACK_FRAME );
  }
  void GamePresentation::CreateDepthState( Errors& errors )
  {
    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = DepthFunc::Less;
    mDepthState = Render::CreateDepthState( "tac 3d depth state", depthStateData, TAC_STACK_FRAME );
  }
  void GamePresentation::CreateBlendState( Errors& errors )
  {
    Render::BlendState blendStateData;
    blendStateData.srcRGB = BlendConstants::One;
    blendStateData.dstRGB = BlendConstants::Zero;
    blendStateData.blendRGB = BlendMode::Add;
    blendStateData.srcA = BlendConstants::Zero;
    blendStateData.dstA = BlendConstants::One;
    blendStateData.blendA = BlendMode::Add;
    mBlendState = Render::CreateBlendState( "tac 3d opaque blend", blendStateData, TAC_STACK_FRAME );
  }
  void GamePresentation::CreateRasterizerState( Errors& errors )
  {
    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = CullMode::None; // todo
    rasterizerStateData.mFillMode = FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( "tac 3d rast state",
                                                      rasterizerStateData,
                                                      TAC_STACK_FRAME );
  }
  void GamePresentation::CreateSamplerState( Errors& errors )
  {
    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Filter::Aniso;
    mSamplerState = Render::CreateSamplerState( "tac 3d tex sampler",
                                                samplerStateData,
                                                TAC_STACK_FRAME );
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
  void GamePresentation::RenderGameWorldAddDrawCall( const Mesh* mesh,
                                                     const DefaultCBufferPerObject& cbuf,
                                                     const Render::ViewId viewId )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      DrawCall2 drawCall = {};
      drawCall.mVertexBuffer = subMesh.mVertexBuffer;
      drawCall.mIndexBuffer = subMesh.mIndexBuffer;
      drawCall.mStartIndex = 0;
      drawCall.mIndexCount = subMesh.mIndexCount;
      drawCall.mBlendState = mBlendState;
      drawCall.mRasterizerState = mRasterizerState;
      drawCall.mSamplerState = mSamplerState;
      drawCall.mDepthState = mDepthState;
      drawCall.mVertexFormat = mesh->mVertexFormat;
      drawCall.mUniformDst = mPerObj;
      drawCall.mUniformSrcc = TemporaryMemoryFromT( cbuf );
      drawCall.mFrame = TAC_STACK_FRAME;
      Render::AddDrawCall( drawCall );

      // ^ old
      // ------------------------------------------------------------
      // ^ new

      Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, 0 );
      Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( mSamplerState );
      Render::SetDepthState( mDepthState );
      Render::SetVertexFormat( mesh->mVertexFormat );
      Render::UpdateConstantBuffer( mPerObj,
                                    &cbuf,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      Render::Submit( viewId );
    }
  }
}
