#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererUtil.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacCamera.h"
#include "src/common/shell/tacShellTimer.h"
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

  static Render::ShaderHandle          m3DShader;
  static Render::ShaderHandle          mTerrainShader;
  static Render::VertexFormatHandle    m3DVertexFormat;
  static Render::VertexFormatHandle    mTerrainVertexFormat;
  static Render::ConstantBufferHandle  mPerFrame;
  static Render::ConstantBufferHandle  mPerObj;
  static Render::DepthStateHandle      mDepthState;
  static Render::BlendStateHandle      mBlendState;
  static Render::RasterizerStateHandle mRasterizerState;
  static Render::SamplerStateHandle    mSamplerState;
  static Render::VertexDeclarations    m3DVertexFormatDecls;
  static Errors                        mGetTextureErrorsGround;
  static Errors                        mGetTextureErrorsNoise;

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


  static void RenderGameWorldAddDrawCall( const Mesh* mesh,
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

  static void LoadTerrain( Terrain* terrain )
  {
    if( terrain->mVertexBuffer.IsValid() && terrain->mIndexBuffer.IsValid() )
      return;

    if( terrain->mRowMajorGrid.empty() )
      return;

    //OSDebugBreak();

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
                                                         Render::Access::Default,
                                                         TAC_STACK_FRAME );

    Render::Format format;
    format.mElementCount = 1;
    format.mPerElementByteCount = sizeof( TerrainIndex );
    format.mPerElementDataType = Render::GraphicsType::uint;
    terrain->mIndexBuffer = Render::CreateIndexBuffer( indexes.size() * sizeof( TerrainIndex ),
                                                       indexes.data(),
                                                       Render::Access::Default,
                                                       format,
                                                       TAC_STACK_FRAME );
    terrain->mIndexCount = indexes.size();

  }

  static void LoadModel( Model* model )
  {
    if( model->mesh )
      return;
    if( model->mModelPath.empty() )
      return;

    Errors getmeshErrors;
    if( model->mTryingNewThing )
    {
      model->mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath,
                                                            model->mModelIndex,
                                                            m3DVertexFormatDecls,
                                                            getmeshErrors );
    }
    else
    {
      model->mesh = ModelAssetManagerGetMesh( model->mModelPath,
                                              m3DVertexFormatDecls,
                                              getmeshErrors );
    }
  }

  void GamePresentationRender( World* world,
                               const Camera* camera,
                               const int viewWidth,
                               const int viewHeight,
                               const Render::ViewHandle viewId )
  {

    TAC_RENDER_GROUP_BLOCK( "Render Game World" );
    //_PROFILE_BLOCK;

    //World* world = world;
    Graphics* graphics = GetGraphics( world );
    Physics* physics = Physics::GetSystem( world );

    float a;
    float b;
    Render::GetPerspectiveProjectionAB( camera->mFarPlane,
                                        camera->mNearPlane,
                                        a,
                                        b );

    const float w = ( float )viewWidth;
    const float h = ( float )viewHeight;
    const float aspect = w / h;
    const m4 view = camera->View();
    const m4 proj = camera->Proj( a, b, aspect );

    const double elapsedSeconds = ShellGetElapsedSeconds();

    DefaultCBufferPerFrame perFrameData;
    perFrameData.mFar = camera->mFarPlane;
    perFrameData.mNear = camera->mNearPlane;
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
        LoadModel( model );
        if( !model->mesh )
          return;

        DefaultCBufferPerObject perObjectData;
        perObjectData.Color = { model->mColorRGB, 1 };
        perObjectData.World = model->mEntity->mWorldTransform;
        Render::BeginGroup( FrameMemoryPrintf( "%s %i", model->mModelPath.c_str(), model->mModelIndex ), TAC_STACK_FRAME );
        RenderGameWorldAddDrawCall( model->mesh, perObjectData, mViewId );
        Render::EndGroup( TAC_STACK_FRAME );
      }

      //GamePresentation*  mGamePresentation;
      Render::ViewHandle mViewId;
    } myModelVisitor;
    //myModelVisitor.mGamePresentation = this;
    myModelVisitor.mViewId = viewId;

    Render::BeginGroup( "Visit Models", TAC_STACK_FRAME );
    graphics->VisitModels( &myModelVisitor );
    Render::EndGroup( TAC_STACK_FRAME );


    Render::BeginGroup( "Visit Terrains", TAC_STACK_FRAME );
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

      Render::SetTexture( Render::DrawCallTextures{ terrainTexture, noiseTexture } );
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
    Render::EndGroup( TAC_STACK_FRAME );


    struct : public SkyboxVisitor
    {
      void operator()( Skybox* skybox ) override
      {
        SkyboxPresentationRender( mCamera,
                                  mViewWidth,
                                  mViewHeight,
                                  mViewId,
                                  skybox->mSkyboxDir );
      }
      int                mViewWidth;
      int                mViewHeight;
      Render::ViewHandle mViewId;
      //GamePresentation*  mGamePresentation;
      const Camera*      mCamera;
    } mySkyboxVisitor;
    mySkyboxVisitor.mViewWidth = viewWidth;
    mySkyboxVisitor.mViewHeight = viewHeight;
    mySkyboxVisitor.mViewId = viewId;
    //mySkyboxVisitor.mGamePresentation = this;
    mySkyboxVisitor.mCamera = camera;
    Render::BeginGroup( "Visit Skyboxes", TAC_STACK_FRAME );
    graphics->VisitSkyboxes( &mySkyboxVisitor );
    Render::EndGroup( TAC_STACK_FRAME );

    //world->mDebug3DDrawData->DrawToTexture(
    //  ignored,
    //  &perFrameData,
    //  mDesktopWindow->mRenderView );
  }

  static void CreateTerrainShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mTerrainShader = Render::CreateShader( Render::ShaderSource::FromPath( "Terrain" ),
                                           Render::ConstantBuffers{ mPerFrame, mPerObj },
                                           TAC_STACK_FRAME );
  }

  static void Create3DShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    m3DShader = Render::CreateShader( Render::ShaderSource::FromPath( "3DTest" ),
                                      Render::ConstantBuffers{ mPerFrame, mPerObj },
                                      TAC_STACK_FRAME );
  }

  static void Create3DVertexFormat( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::VertexDeclaration posDecl;
    posDecl.mAlignedByteOffset = 0;
    posDecl.mAttribute = Render::Attribute::Position;
    posDecl.mTextureFormat.mElementCount = 3;
    posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    posDecl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    m3DVertexFormatDecls = Render::VertexDeclarations{ posDecl };
    m3DVertexFormat = Render::CreateVertexFormat( m3DVertexFormatDecls,
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
  }

  static void CreateTerrainVertexFormat( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::VertexDeclaration terrainPosDecl = {};
    terrainPosDecl.mAttribute = Render::Attribute::Position;
    terrainPosDecl.mTextureFormat.mElementCount = 3;
    terrainPosDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainPosDecl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    terrainPosDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mPos );

    Render::VertexDeclaration terrainNorDecl = {};
    terrainNorDecl.mAttribute = Render::Attribute::Normal;
    terrainNorDecl.mTextureFormat.mElementCount = 3;
    terrainNorDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainNorDecl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    terrainNorDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mNor );

    Render::VertexDeclaration terrainTexCoordDecl = {};
    terrainTexCoordDecl.mAttribute = Render::Attribute::Texcoord;
    terrainTexCoordDecl.mTextureFormat.mElementCount = 2;
    terrainTexCoordDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    terrainTexCoordDecl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
    terrainTexCoordDecl.mAlignedByteOffset = TAC_OFFSET_OF( TerrainVertex, mUV );

    mTerrainVertexFormat = Render::CreateVertexFormat( { terrainPosDecl, terrainNorDecl, terrainTexCoordDecl },
                                                       mTerrainShader,
                                                       TAC_STACK_FRAME );
  }

  static void CreatePerFrame( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mPerFrame = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerFrame ),
                                              0,
                                              TAC_STACK_FRAME );
  }

  static void CreatePerObj( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mPerObj = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerObject ),
                                            1,
                                            TAC_STACK_FRAME );
  }

  static void CreateDepthState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = Render::DepthFunc::Less;
    mDepthState = Render::CreateDepthState( depthStateData, TAC_STACK_FRAME );
  }

  static void CreateBlendState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::BlendState blendStateData;
    blendStateData.mSrcRGB = Render::BlendConstants::One;
    blendStateData.mDstRGB = Render::BlendConstants::Zero;
    blendStateData.mBlendRGB = Render::BlendMode::Add;
    blendStateData.mSrcA = Render::BlendConstants::Zero;
    blendStateData.mDstA = Render::BlendConstants::One;
    blendStateData.mBlendA = Render::BlendMode::Add;
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
  }

  static void CreateRasterizerState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = Render::CullMode::None; // todo
    rasterizerStateData.mFillMode = Render::FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData,
                                                      TAC_STACK_FRAME );
  }

  static void CreateSamplerState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Render::Filter::Aniso;
    mSamplerState = Render::CreateSamplerState( samplerStateData,
                                                TAC_STACK_FRAME );
  }

  void GamePresentationInit( Errors& errors )
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

  void GamePresentationUninit()
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

}
