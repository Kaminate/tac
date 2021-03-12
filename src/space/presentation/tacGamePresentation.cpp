#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacCamera.h"
#include "src/common/tacShellTimer.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacMemory.h"
#include "src/common/tacOS.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/skybox/tacSkyboxComponent.h"
#include "src/space/model/tacModel.h"
#include "src/space/physics/tacPhysics.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacSkyboxPresentation.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"
#include "src/space/terrain/tacTerrain.h"

#include <cmath> // std::fmod

namespace Tac
{
  struct TerrainVertex
  {
    v3 mPos;
    v3 mNor;
    v2 mUV;
  };

  static TerrainVertex GetTerrainVertex( const Terrain* terrain,
                                         const int r,
                                         const int c,
                                         const v2 uv )
  {
    TerrainVertex terrainVertex = {};
    terrainVertex.mPos = terrain->GetGridVal( r, c );
    terrainVertex.mNor = terrain->GetGridValNormal( r, c );
    terrainVertex.mUV = uv;
    return terrainVertex;
  }

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


  void GamePresentation::LoadTerrain( Terrain* terrain )
  {
    if( terrain->mVertexBuffer.IsValid() && terrain->mIndexBuffer.IsValid() )
      return;

    if( terrain->mRowMajorGrid.empty() )
      return;

    //OS::DebugBreak();

    typedef uint32_t TerrainIndex;


    Vector< TerrainVertex > vertexes;
    Vector< TerrainIndex > indexes;

    for( int iRow = 1; iRow < terrain->mSideVertexCount; ++iRow )
    {
      for( int iCol = 1; iCol < terrain->mSideVertexCount; ++iCol )
      {
        const int iVertexTL = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow - 1, iCol - 1, { 0, 1 } ) );

        const int iVertexTR = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow - 1, iCol, { 1, 1 } ) );

        const int iVertexBL = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow, iCol - 1, { 0, 0 } ) );

        const int iVertexBR = vertexes.size();
        vertexes.push_back( GetTerrainVertex( terrain, iRow, iCol, { 1, 0 } ) );

        // lower left triangle
        indexes.push_back( iVertexBR );
        indexes.push_back( iVertexTL );
        indexes.push_back( iVertexBL );

        // upper right triangle
        indexes.push_back( iVertexBR );
        indexes.push_back( iVertexTR );
        indexes.push_back( iVertexTL );
      }
    }

    terrain->mVertexBuffer = Render::CreateVertexBuffer( vertexes.size() * sizeof( TerrainVertex ),
                                                         vertexes.data(),
                                                         sizeof( TerrainVertex ),
                                                         Access::Default,
                                                         TAC_STACK_FRAME );

    Format format;
    format.mElementCount = 1;
    format.mPerElementByteCount = sizeof( TerrainIndex );
    format.mPerElementDataType = GraphicsType::uint;
    terrain->mIndexBuffer = Render::CreateIndexBuffer( indexes.size() * sizeof( TerrainIndex ),
                                                       indexes.data(),
                                                       Access::Default,
                                                       format,
                                                       TAC_STACK_FRAME );
    terrain->mIndexCount = indexes.size();

  }

  void GamePresentation::LoadModel( Model* model )
  {
    if( model->mesh )
      return;
    if( model->mModelPath.empty() )
      return;
    Errors getmeshErrors;
    ModelAssetManagerGetMesh( &model->mesh,
                              model->mModelPath,
                              m3DVertexFormatDecls,
                              getmeshErrors );
  }

  void GamePresentation::RenderGameWorldToDesktopView( const int viewWidth,
                                                       const int viewHeight,
                                                       const Render::ViewHandle viewId )
  {
    //_PROFILE_BLOCK;

    //World* world = mWorld;
    Graphics* graphics = GetGraphics( mWorld );
    Physics* physics = Physics::GetSystem( mWorld );

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

    const double elapsedSeconds = ShellGetElapsedSeconds();

    DefaultCBufferPerFrame perFrameData;
    perFrameData.mFar = mCamera->mFarPlane;
    perFrameData.mNear = mCamera->mNearPlane;
    perFrameData.mView = view;
    perFrameData.mProjection = proj;
    perFrameData.mGbufferSize = { w, h };
    perFrameData.mSecModTau = ( float )std::fmod( elapsedSeconds, 6.2831853 );

    Render::UpdateConstantBuffer( mPerFrame,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );

    Render::Submit( viewId, TAC_STACK_FRAME );

    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        mGamePresentation->LoadModel( model );
        if( !model->mesh )
          return;

        DefaultCBufferPerObject perObjectData;
        perObjectData.Color = { model->mColorRGB, 1 };
        perObjectData.World = model->mEntity->mWorldTransform;
        mGamePresentation->RenderGameWorldAddDrawCall( model->mesh, perObjectData, mViewId );
      }

      GamePresentation*  mGamePresentation;
      Render::ViewHandle mViewId;
    } myModelVisitor;
    myModelVisitor.mGamePresentation = this;
    myModelVisitor.mViewId = viewId;
    graphics->VisitModels( &myModelVisitor );


    for( Terrain* terrain : physics->mTerrains )
    {
      LoadTerrain( terrain );

      if( !terrain->mVertexBuffer.IsValid() || !terrain->mIndexBuffer.IsValid() )
        continue;

      const Render::TextureHandle terrainTexture =
        TextureAssetManager::GetTexture( terrain->mGroundTexturePath, mGetTextureErrorsGround );
      const Render::TextureHandle noiseTexture =
        TextureAssetManager::GetTexture( terrain->mNoiseTexturePath, mGetTextureErrorsNoise );

      DefaultCBufferPerObject cbuf = {};
      cbuf.Color = { 1, 1, 1, 1 };
      cbuf.World = m4::Identity();

      Render::SetTexture( Render::DrawCallTextures( terrainTexture, noiseTexture ) );
      Render::UpdateConstantBuffer( mPerObj, &cbuf, sizeof( DefaultCBufferPerObject ), TAC_STACK_FRAME );
      Render::SetDepthState( mDepthState );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( mSamplerState );
      Render::SetShader( mTerrainShader );
      Render::SetIndexBuffer( terrain->mIndexBuffer, 0, terrain->mIndexCount );
      Render::SetVertexBuffer( terrain->mVertexBuffer, 0, 0 );
      Render::SetVertexFormat( mTerrainVertexFormat );
      Render::Submit( viewId, TAC_STACK_FRAME );
    }


    struct : public SkyboxVisitor
    {
      void operator()( Skybox* skybox ) override
      {
        mGamePresentation->mSkyboxPresentation->RenderSkybox( mViewWidth,
                                                              mViewHeight,
                                                              mViewId,
                                                              skybox->mSkyboxDir );
      }
      int                mViewWidth;
      int                mViewHeight;
      Render::ViewHandle mViewId;
      GamePresentation*  mGamePresentation;
    } mySkyboxVisitor;
    mySkyboxVisitor.mViewId = viewId;
    mySkyboxVisitor.mViewWidth = viewWidth;
    mySkyboxVisitor.mViewHeight = viewHeight;
    mySkyboxVisitor.mGamePresentation = this;
    graphics->VisitSkyboxes( &mySkyboxVisitor );

    //world->mDebug3DDrawData->DrawToTexture(
    //  ignored,
    //  &perFrameData,
    //  mDesktopWindow->mRenderView );
  }

  void GamePresentation::CreateTerrainShader( Errors& errors )
  {
    mTerrainShader = Render::CreateShader( Render::ShaderSource::FromPath( "Terrain" ),
                                           Render::ConstantBuffers( mPerFrame, mPerObj ),
                                           TAC_STACK_FRAME );
  }

  void GamePresentation::Create3DShader( Errors& errors )
  {
    m3DShader = Render::CreateShader( Render::ShaderSource::FromPath( "3DTest" ),
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
    m3DVertexFormatDecls.AddVertexDeclaration( posDecl );
    m3DVertexFormat = Render::CreateVertexFormat( m3DVertexFormatDecls,
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
  }

  void GamePresentation::CreateTerrainVertexFormat( Errors& errors )
  {
    VertexDeclaration terrainPosDecl = {};
    terrainPosDecl.mAttribute = Attribute::Position;
    terrainPosDecl.mTextureFormat.mElementCount = 3;
    terrainPosDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainPosDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
    terrainPosDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mPos );

    VertexDeclaration terrainNorDecl = {};
    terrainNorDecl.mAttribute = Attribute::Normal;
    terrainNorDecl.mTextureFormat.mElementCount = 3;
    terrainNorDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainNorDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
    terrainNorDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mNor );

    VertexDeclaration terrainTexCoordDecl = {};
    terrainTexCoordDecl.mAttribute = Attribute::Texcoord;
    terrainTexCoordDecl.mTextureFormat.mElementCount = 2;
    terrainTexCoordDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainTexCoordDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
    terrainTexCoordDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mUV );

    VertexDeclarations vertexDeclarations;
    vertexDeclarations.AddVertexDeclaration( terrainPosDecl );
    vertexDeclarations.AddVertexDeclaration( terrainNorDecl );
    vertexDeclarations.AddVertexDeclaration( terrainTexCoordDecl );

    mTerrainVertexFormat = Render::CreateVertexFormat( vertexDeclarations,
                                                       mTerrainShader,
                                                       TAC_STACK_FRAME );
  }

  void GamePresentation::CreatePerFrame( Errors& errors )
  {
    mPerFrame = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerFrame ),
                                              0,
                                              TAC_STACK_FRAME );
  }

  void GamePresentation::CreatePerObj( Errors& errors )
  {
    mPerObj = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerObject ),
                                            1,
                                            TAC_STACK_FRAME );
  }

  void GamePresentation::CreateDepthState( Errors& errors )
  {
    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = DepthFunc::Less;
    mDepthState = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
  }

  void GamePresentation::CreateBlendState( Errors& errors )
  {
    Render::BlendState blendStateData;
    blendStateData.mSrcRGB = BlendConstants::One;
    blendStateData.mDstRGB = BlendConstants::Zero;
    blendStateData.mBlendRGB = BlendMode::Add;
    blendStateData.mSrcA = BlendConstants::Zero;
    blendStateData.mDstA = BlendConstants::One;
    blendStateData.mBlendA = BlendMode::Add;
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
  }

  void GamePresentation::CreateRasterizerState( Errors& errors )
  {
    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = CullMode::None; // todo
    rasterizerStateData.mFillMode = FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData,
                                                      TAC_STACK_FRAME );
  }

  void GamePresentation::CreateSamplerState( Errors& errors )
  {
    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Filter::Aniso;
    mSamplerState = Render::CreateSamplerState( samplerStateData,
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
                                                     const Render::ViewHandle viewId )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      Render::SetShader( m3DShader );
      Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, 0 );
      Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( mSamplerState );
      Render::SetDepthState( mDepthState );
      //Render::SetVertexFormat( mesh->mVertexFormat );
      Render::SetVertexFormat( m3DVertexFormat );
      Render::UpdateConstantBuffer( mPerObj,
                                    &cbuf,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      Render::Submit( viewId, TAC_STACK_FRAME );
    }
  }
}
