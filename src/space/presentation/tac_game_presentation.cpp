#include "src/common/assetmanagers/tac_mesh.h"
#include "src/common/assetmanagers/tac_model_asset_manager.h"
#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_renderer_util.h"
#include "src/common/math/tac_math.h"
#include "src/common/math/tac_matrix4.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/math/tac_vector4.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/system/tac_os.h"
#include "src/space/graphics/tac_graphics.h"
#include "src/space/light/tac_light.h"
#include "src/space/model/tac_model.h"
#include "src/space/physics/tac_physics.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/presentation/tac_shadow_presentation.h"
#include "src/space/presentation/tac_skybox_presentation.h"
#include "src/space/skybox/tac_skybox_component.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"
#include "src/space/terrain/tac_terrain.h"

#include <cmath> // std::fmod

namespace Tac
{
  static Render::ShaderHandle          m3DShader;
  static Render::ShaderHandle          mTerrainShader;
  static Render::VertexFormatHandle    m3DVertexFormat;
  static Render::VertexFormatHandle    mTerrainVertexFormat;
  static Render::DepthStateHandle      mDepthState;
  static Render::BlendStateHandle      mBlendState;
  static Render::RasterizerStateHandle mRasterizerState;
  static Render::SamplerStateHandle    mSamplerStateAniso;
  static Render::SamplerStateHandle    mSamplerStatePointShadow;
  static Render::VertexDeclarations    m3DVertexFormatDecls;
  static Errors                        mGetTextureErrorsGround;
  static Errors                        mGetTextureErrorsNoise;
  static bool                          mRenderEnabledModel = true;
  static bool                          mRenderEnabledSkybox = true;
  static bool                          mRenderEnabledTerrain = true;
  static bool                          mRenderEnabledDebug3D = true;
  static bool                          mUseLights = true;
  static CBufferLights                 mDebugCBufferLights = {};

  struct TerrainVertex
  {
    v3 mPos;
    v3 mNor;
    v2 mUV;
  };

  struct GameModelVtx
  {
    v3 mPos;
    v3 mNor;
  };

  static void CheckShaderPadding()
  {
    const int sizeofshaderlight_estimated = 4 * ( 16 + 4 + 4 + 4 + 1 + 3 );
    const int sizeofshaderlight = sizeof( ShaderLight );
    const int light1Offset = ( int )TAC_OFFSET_OF( CBufferLights, lights[ 1 ] );
    const int light1OffsetReg = light1Offset / 16;
    const int light1OffsetAxis = light1Offset % 16;
    const int lightCountOffset = ( int )TAC_OFFSET_OF( CBufferLights, lightCount );
    const int lightCountOffsetReg = lightCountOffset / 16;
    const int lightCountOffsetAxis = lightCountOffset % 16;
    const bool check1 = sizeofshaderlight % 16 == 0;
    const bool check2 = sizeofshaderlight == sizeofshaderlight_estimated;
    const bool check3 = light1OffsetReg == 8;
    const bool check4 = light1OffsetAxis == 0;
    const bool check5 = lightCountOffsetReg == 32;
    const bool check6 = lightCountOffsetAxis == 0;
    TAC_ASSERT( check1 );
    TAC_ASSERT( check2 );
    TAC_ASSERT( check3 );
    TAC_ASSERT( check4 );
    TAC_ASSERT( check5 );
    TAC_ASSERT( check6 );
  }

  static DefaultCBufferPerFrame GetPerFrameBuf( const Camera* camera,
                                                const int viewWidth,
                                                const int viewHeight )
  {
    const double elapsedSeconds = ShellGetElapsedSeconds();
    float a;
    float b;
    Render::GetPerspectiveProjectionAB( camera->mFarPlane,
                                        camera->mNearPlane,
                                        a,
                                        b );
    const float w = ( float )viewWidth;
    const float h = ( float )viewHeight;
    return { .mView = camera->View(),
             .mProjection = camera->Proj( a, b, w / h ),
             .mFar = camera->mFarPlane,
             .mNear = camera->mNearPlane,
             .mGbufferSize = { w, h },
             .mSecModTau = ( float )Fmod( elapsedSeconds, 6.2831853 )};
  }

  static TerrainVertex GetTerrainVertex( const Terrain* terrain,
                                         const int r,
                                         const int c,
                                         const v2 uv )
  {
    return { .mPos = terrain->GetGridVal( r, c ),
             .mNor = terrain->GetGridValNormal( r, c ),
             .mUV = uv};
  }

  static void Debug3DEachTri( Graphics* graphics )
  {
    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        Errors errors;
        Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                             model->mModelIndex,
                                                             GamePresentationGetVertexDeclarations(),
                                                             errors );
        if( !mesh )
          return;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          for( const SubMeshTriangle& tri : subMesh.mTris )
          {
            const v3 p0 = ( model->mEntity->mWorldTransform *  v4( tri[ 0 ], 1 ) ).xyz();
            const v3 p1 = ( model->mEntity->mWorldTransform *  v4( tri[ 1 ], 1 ) ).xyz();
            const v3 p2 = ( model->mEntity->mWorldTransform *  v4( tri[ 2 ], 1 ) ).xyz();
            mDrawData->DebugDraw3DTriangle( p0, p1, p2 );
          }
        }
      }
      Debug3DDrawData* mDrawData = nullptr;
    } visitor = {};
    visitor.mDrawData = graphics->mWorld->mDebug3DDrawData;

    graphics->VisitModels( &visitor );
  }

  static Mesh* LoadModel( const Model* model )
  {
    Errors getmeshErrors;
    return ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                   model->mModelIndex,
                                                   m3DVertexFormatDecls,
                                                   getmeshErrors );
  }

  static void RenderGameWorldAddDrawCall( const Light** lights,
                                          int lightCount,
                                          const Model* model,
                                          const Render::ViewHandle viewId )
  {
    const Mesh* mesh = LoadModel( model );
    if( !mesh )
      return;

    const DefaultCBufferPerObject perObjectData
    {
      .World = model->mEntity->mWorldTransform,
      .Color = PremultipliedAlpha::From_sRGB( model->mColorRGB ),
    };

    Render::DrawCallTextures drawCallTextures;
    CBufferLights cBufferLights;
    if( mUseLights )
    {
      for( int i = 0; i < lightCount; ++i )
      {
        const Light* light = lights[ i ];
        if( cBufferLights.TryAddLight( LightToShaderLight( light ) ) )
          drawCallTextures.push_back( light->mShadowMapDepth );
      }
    }
    mDebugCBufferLights = cBufferLights;

    Render::SetTexture( drawCallTextures );

    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      const  char* groupName = FrameMemoryPrintf( "%s %s", model->mEntity->mName.c_str(), subMesh.mName.c_str() );
      Render::BeginGroup( groupName, TAC_STACK_FRAME );
      //FrameMemoryPrintf( "%s %i", subMesh.mName.c_str(),
      //                                         model->mModelPath.c_str(),
      //                                         model->mModelIndex ), TAC_STACK_FRAME );
      Render::SetShader( m3DShader );
      Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
      Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( { mSamplerStateAniso, mSamplerStatePointShadow } );
      Render::SetDepthState( mDepthState );
      Render::SetVertexFormat( m3DVertexFormat );
      Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      Render::UpdateConstantBuffer( DefaultCBufferPerObject::Handle,
                                    &perObjectData,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      Render::Submit( viewId, TAC_STACK_FRAME );
      Render::EndGroup( TAC_STACK_FRAME );
    }
  }

  static void LoadTerrain( Terrain* terrain )
  {
    if( terrain->mVertexBuffer.IsValid() && terrain->mIndexBuffer.IsValid() )
      return;

    if( terrain->mRowMajorGrid.empty() )
      return;


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
    Render::SetRenderObjectDebugName( terrain->mVertexBuffer, "terrain-vtx-buf" );

    const Render::Format format{ .mElementCount = 1,
                                  .mPerElementByteCount = sizeof( TerrainIndex ),
                                  .mPerElementDataType = Render::GraphicsType::uint };
    terrain->mIndexBuffer = Render::CreateIndexBuffer( indexes.size() * sizeof( TerrainIndex ),
                                                       indexes.data(),
                                                       Render::Access::Default,
                                                       format,
                                                       TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( terrain->mIndexBuffer, "terrain-idx-buf" );
    terrain->mIndexCount = indexes.size();

  }

  static void CreateTerrainShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mTerrainShader = Render::CreateShader(  "Terrain" , TAC_STACK_FRAME );
  }

  static void Create3DShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    m3DShader = Render::CreateShader(  "GamePresentation" , TAC_STACK_FRAME );
  }

  static void Create3DVertexFormat( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );

    const Render::VertexDeclaration posDecl {
      .mAttribute = Render::Attribute::Position,
      .mTextureFormat{.mElementCount = 3,
                     .mPerElementByteCount = sizeof( float ),
                     .mPerElementDataType = Render::GraphicsType::real},
      .mAlignedByteOffset = (int)TAC_OFFSET_OF( GameModelVtx, mPos ),};

    const Render::VertexDeclaration norDecl 
    {
      .mAttribute = Render::Attribute::Normal,
      .mTextureFormat{.mElementCount = 3,
                     .mPerElementByteCount = sizeof( float ),
                     .mPerElementDataType = Render::GraphicsType::real},
      .mAlignedByteOffset = (int)TAC_OFFSET_OF( GameModelVtx, mNor ),
    };

    m3DVertexFormatDecls = Render::VertexDeclarations{ posDecl, norDecl };
    m3DVertexFormat = Render::CreateVertexFormat( m3DVertexFormatDecls, m3DShader, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( m3DVertexFormat, "game-3d-vtx-fmt" );
  }

  static void CreateTerrainVertexFormat( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    const Render::VertexDeclarations decls
    {
      {
        .mAttribute = Render::Attribute::Position,
        .mTextureFormat{ .mElementCount = 3,
                         .mPerElementByteCount = sizeof( float ),
                         .mPerElementDataType = Render::GraphicsType::real},
        .mAlignedByteOffset = (int)TAC_OFFSET_OF( TerrainVertex, mPos )
      },
      {
        .mAttribute = Render::Attribute::Normal,
        .mTextureFormat{ .mElementCount = 3,
                         .mPerElementByteCount = sizeof( float ),
                         .mPerElementDataType = Render::GraphicsType::real },
        .mAlignedByteOffset = (int)TAC_OFFSET_OF( TerrainVertex, mNor )
      },
      {
        .mAttribute = Render::Attribute::Texcoord,
        .mTextureFormat{ .mElementCount = 2,
                         .mPerElementByteCount = sizeof( float ),
                         .mPerElementDataType = Render::GraphicsType::real},
        .mAlignedByteOffset = (int)TAC_OFFSET_OF( TerrainVertex, mUV )
      }
    };
    mTerrainVertexFormat = Render::CreateVertexFormat( decls, mTerrainShader, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mTerrainVertexFormat, "terrain-vtx-fmt" );
  }

  static void CreateDepthState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mDepthState = Render::CreateDepthState( { .mDepthTest = true,
                                              .mDepthWrite = true,
                                              .mDepthFunc = Render::DepthFunc::Less},
                                            TAC_STACK_FRAME );
  }

  static void CreateBlendState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mBlendState = Render::CreateBlendState( { .mSrcRGB = Render::BlendConstants::One,
                                              .mDstRGB = Render::BlendConstants::Zero,
                                              .mBlendRGB = Render::BlendMode::Add,
                                              .mSrcA = Render::BlendConstants::Zero,
                                              .mDstA = Render::BlendConstants::One,
                                              .mBlendA = Render::BlendMode::Add},
                                            TAC_STACK_FRAME );
  }

  static void CreateRasterizerState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mRasterizerState = Render::CreateRasterizerState( { .mFillMode = Render::FillMode::Solid,
                                                        .mCullMode = Render::CullMode::Back,
                                                        .mFrontCounterClockwise = true,
                                                        .mScissor = true,
                                                        .mMultisample = false},
                                                      TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerState, "game-pres-rast" );

  }

  static void CreateSamplerStateShadow( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mSamplerStatePointShadow = Render::CreateSamplerState( { .mFilter = Render::Filter::Point }, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerStateAniso, "game-shadow-samp" );
  }

  static void CreateSamplerState( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mSamplerStateAniso = Render::CreateSamplerState( { .mFilter = Render::Filter::Aniso }, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerStateAniso, "game-samp" );
  }

  static void RenderModels( World* world,
                            const Camera* camera,
                            const int viewWidth,
                            const int viewHeight,
                            const Render::ViewHandle viewId )
  {
    if( !mRenderEnabledModel )
      return;
    const DefaultCBufferPerFrame perFrameData = GetPerFrameBuf( camera,
                                                                viewWidth,
                                                                viewHeight );
    Render::UpdateConstantBuffer( DefaultCBufferPerFrame::Handle,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );

    Graphics* graphics = GetGraphics( world );
    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        static struct : public LightVisitor
        {
          void operator()( Light* light ) override { mLights.push_back( light ); }
          Vector< const Light* > mLights;
        } sLightVisitor;

        sLightVisitor.mLights.resize( 0 );

        if( mUseLights )
          mGraphics->VisitLights( &sLightVisitor );

        sLightVisitor.mLights.resize( Min( sLightVisitor.mLights.size(), CBufferLights::TAC_MAX_SHADER_LIGHTS ) );


        RenderGameWorldAddDrawCall( sLightVisitor.mLights.data(),
                                    sLightVisitor.mLights.size(),
                                    model,
                                    mViewId );
      }

      Render::ViewHandle mViewId;
      //World*             mWorld;
      Graphics*          mGraphics = nullptr;
    } myModelVisitor;
    myModelVisitor.mViewId = viewId;
    myModelVisitor.mGraphics = graphics;
    //myModelVisitor.mWorld = world;



    TAC_RENDER_GROUP_BLOCK( "Visit Models" );
    //Render::BeginGroup( "Visit Models", TAC_STACK_FRAME );
    graphics->VisitModels( &myModelVisitor );
    //Render::EndGroup( TAC_STACK_FRAME );
  }

  static void RenderTerrain( World* world,
                             const Camera* camera,
                             const int viewWidth,
                             const int viewHeight,
                             const Render::ViewHandle viewId )
  {
    if( !mRenderEnabledTerrain )
      return;
    const DefaultCBufferPerFrame perFrameData = GetPerFrameBuf( camera,
                                                                viewWidth,
                                                                viewHeight );
    Render::UpdateConstantBuffer( DefaultCBufferPerFrame::Handle,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );
    Physics* physics = Physics::GetSystem( world );

    TAC_RENDER_GROUP_BLOCK( "Visit Terrains" );
    //Render::BeginGroup( "Visit Terrains", TAC_STACK_FRAME );
    for( Terrain* terrain : physics->mTerrains )
    {
      LoadTerrain( terrain );

      if( !terrain->mVertexBuffer.IsValid() || !terrain->mIndexBuffer.IsValid() )
        continue;

      const Render::TextureHandle terrainTexture =
        TextureAssetManager::GetTexture( terrain->mGroundTexturePath, mGetTextureErrorsGround );
      const Render::TextureHandle noiseTexture =
        TextureAssetManager::GetTexture( terrain->mNoiseTexturePath, mGetTextureErrorsNoise );

      const DefaultCBufferPerObject cbuf;

      Render::SetTexture( Render::DrawCallTextures{ terrainTexture, noiseTexture } );
      Render::UpdateConstantBuffer( DefaultCBufferPerObject::Handle,
                                    &cbuf,
                                    sizeof( DefaultCBufferPerObject ),
                                    TAC_STACK_FRAME );
      Render::SetDepthState( mDepthState );
      Render::SetBlendState( mBlendState );
      Render::SetRasterizerState( mRasterizerState );
      Render::SetSamplerState( mSamplerStateAniso );
      Render::SetShader( mTerrainShader );
      Render::SetIndexBuffer( terrain->mIndexBuffer, 0, terrain->mIndexCount );
      Render::SetVertexBuffer( terrain->mVertexBuffer, 0, 0 );
      Render::SetVertexFormat( mTerrainVertexFormat );
      Render::Submit( viewId, TAC_STACK_FRAME );
    }
    //Render::EndGroup( TAC_STACK_FRAME );
  }


  static void RenderSkybox( World* world,
                            const Camera* camera,
                            const int viewWidth,
                            const int viewHeight,
                            const Render::ViewHandle viewId )
  {

    if( !mRenderEnabledSkybox )
      return;
    Graphics* graphics = GetGraphics( world );
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
      int                mViewWidth = 0;
      int                mViewHeight = 0;
      Render::ViewHandle mViewId;
      const Camera*      mCamera = nullptr;
    } mySkyboxVisitor;
    mySkyboxVisitor.mViewWidth = viewWidth;
    mySkyboxVisitor.mViewHeight = viewHeight;
    mySkyboxVisitor.mViewId = viewId;
    mySkyboxVisitor.mCamera = camera;
    TAC_RENDER_GROUP_BLOCK( "Visit Skyboxes" );
    graphics->VisitSkyboxes( &mySkyboxVisitor );
  }

  const Mesh* GamePresentationGetModelMesh( const Model* model )
  {
    return LoadModel( model );
  }

  void        GamePresentationInit( Errors& errors )
  {

    CheckShaderPadding();

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

    CreateSamplerStateShadow( errors );
  }

  void        GamePresentationUninit()
  {
    Render::DestroyShader( m3DShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( m3DVertexFormat, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
    Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerStateAniso, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerStatePointShadow, TAC_STACK_FRAME );
  }


  void        GamePresentationRender( World* world,
                                      const Camera* camera,
                                      const int viewWidth,
                                      const int viewHeight,
                                      const Render::ViewHandle viewId )
  {
    TAC_RENDER_GROUP_BLOCK( "GamePresentationRender" );

    ShadowPresentationRender( world );

    RenderModels( world, camera, viewWidth, viewHeight, viewId );

    RenderTerrain( world, camera, viewWidth, viewHeight, viewId );

    // Skybox should be last to reduce pixel shader invocations
    RenderSkybox( world,
                  camera,
                  viewWidth,
                  viewHeight,
                  viewId );

    if( mRenderEnabledDebug3D )
    {
      Errors e;
      world->mDebug3DDrawData->DebugDraw3DToTexture( viewId, camera, viewWidth, viewHeight, e );
    }
  }

  Render::DepthStateHandle      GamePresentationGetDepthState()           { return mDepthState; }
  Render::BlendStateHandle      GamePresentationGetBlendState()           { return mBlendState; }
  Render::RasterizerStateHandle GamePresentationGetRasterizerState()      { return mRasterizerState; }
  Render::SamplerStateHandle    GamePresentationGetSamplerState()         { return mSamplerStateAniso; }
  Render::VertexDeclarations    GamePresentationGetVertexDeclarations()   { return m3DVertexFormatDecls; }
  Render::VertexFormatHandle    GamePresentationGetVertexFormat()         { return m3DVertexFormat; }


  void                          GamePresentationDebugImGui( Graphics* graphics )
  {
    if( !ImGuiCollapsingHeader( "Game Presentation" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;
    ImGuiCheckbox( "Game Presentation Enabled Model", &mRenderEnabledModel );
    ImGuiCheckbox( "Game Presentation Enabled Skybox", &mRenderEnabledSkybox );
    ImGuiCheckbox( "Game Presentation Enabled Terrain", &mRenderEnabledTerrain );
    ImGuiCheckbox( "Game Presentation Enabled Debug3D", &mRenderEnabledDebug3D );

    ImGuiCheckbox( "Game Presentation use lights", &mUseLights );
    if( mUseLights && ImGuiCollapsingHeader( "CBufferLights" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiTextf(  "light count %i", mDebugCBufferLights.lightCount  );
      ImGuiTextf(  "text number %i", mDebugCBufferLights.testNumber  );
      for( int iLight = 0; iLight < ( int )mDebugCBufferLights.lightCount; ++iLight )
      {
        if( !ImGuiCollapsingHeader( FrameMemoryPrintf( "Light %i", iLight ) ) )
          continue;

        ShaderLight* shaderLight = &mDebugCBufferLights.lights[ iLight ];
        String rowsStrs[ 4 ];
        for( int r = 0; r < 4; ++r )
          for( int c = 0; c < 4; ++c )
            rowsStrs[ r ] += FrameMemoryPrintf( "%.2f ", shaderLight->mWorldToClip( r, c ) );

        const Light::Type lightType = ( Light::Type )GetShaderLightFlagType()->Extract( shaderLight->mFlags );
        const bool castsShadows = ( bool )GetShaderLightFlagCastsShadows()->Extract( shaderLight->mFlags );

        TAC_IMGUI_INDENT_BLOCK;
        ImGuiText( "World to clip matrix" );
        ImGuiText( rowsStrs[ 0 ] );
        ImGuiText( rowsStrs[ 1 ] );
        ImGuiText( rowsStrs[ 2 ] );
        ImGuiText( rowsStrs[ 3 ] );
        ImGuiDragFloat4( "worldspace pos", shaderLight->mWorldSpacePosition.data() );
        ImGuiDragFloat3( "worldspace pos", shaderLight->mWorldSpacePosition.data() );
        ImGuiDragFloat3( "worldspace unit dir", shaderLight->mWorldSpaceUnitDirection.data() );
        ImGuiDragFloat3( "light color", shaderLight->mColorRadiance.data() );
        ImGuiDragFloat( "light radiance", &shaderLight->mColorRadiance.w );
        //ImGuiDragFloat( "light near plane", &shaderLight->mNear );
        //ImGuiDragFloat( "light far plane", &shaderLight->mFar );
        ImGuiDragFloat( "light proj a", &shaderLight->mProjA );
        ImGuiDragFloat( "light proj b", &shaderLight->mProjB );
        ImGuiImage( -1, v2( 1, 1 ) * 50, v4( shaderLight->mColorRadiance.xyz(), 1.0f ) );
        ImGuiTextf( "Light type: %s (%i)",
                    LightTypeToString( lightType ),
                    ( int )lightType );
        ImGuiTextf( "casts shadows: %s", ( castsShadows ? "true" : "false" ) );
      }
    }

    if( mRenderEnabledDebug3D )
    {
      static bool debugEachTri;
      ImGuiCheckbox( "debug each tri", &debugEachTri );
      if( debugEachTri )
      {
        Debug3DEachTri( graphics );
      }
    }
  }
}
