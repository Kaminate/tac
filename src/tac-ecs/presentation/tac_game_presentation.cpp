#include "tac_game_presentation.h" // self-inc

#include "tac-std-lib/tac_ints.h"

#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/presentation/tac_shadow_presentation.h"
#include "tac-ecs/presentation/tac_skybox_presentation.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/terrain/tac_terrain.h"


#if TAC_GAME_PRESENTATION_ENABLED()

namespace Tac
{
  struct TerrainConstBuf
  {
    m4 mWorld;
    m4 mView;
    m4 mProj;
  };

  static Render::BufferHandle          mTerrainConstBuf;
  static Render::ProgramHandle         m3DShader;
  static Render::ProgramHandle         mTerrainShader;
  static Render::PipelineHandle        mTerrainPipeline;
  static Render::PipelineHandle        mGamePipeline;
  //static Render::VertexFormatHandle    m3DVertexFormat;
  //static Render::VertexFormatHandle    mTerrainVertexFormat;
  //static Render::DepthStateHandle      mDepthState;
  //static Render::BlendStateHandle      mBlendState;
  //static Render::RasterizerStateHandle mRasterizerState;
  static Render::SamplerHandle         mSamplerPoint;
  static Render::SamplerHandle         mSamplerLinear;
  static Render::SamplerHandle         mSamplerAniso;
  static Render::VertexDeclarations    m3DVertexFormatDecls;
  static Render::VertexDeclarations    mTerrainVtxDecls;
  static Render::IShaderVar*           mShaderTerrainHeightmap;
  static Render::IShaderVar*           mShaderTerrainNoise;
  static Render::IShaderVar*           mShaderTerrainSampler;
  static Render::IShaderVar*           mShaderTerrainConstBuf;
  static Render::IShaderVar*           mShaderGameShadowMaps;
  static Render::IShaderVar*           mShaderGameLinearSampler;
  static Render::IShaderVar*           mShaderGameShadowSampler;
  static Render::IShaderVar*           mShaderGamePerFrame;
  static Render::IShaderVar*           mShaderGamePerObject;
  static Errors                        mGetTextureErrorsGround;
  static Errors                        mGetTextureErrorsNoise;
  static bool                          mRenderEnabledModel{ true };
  static bool                          mRenderEnabledSkybox{ true };
  static bool                          mRenderEnabledTerrain{ true };
  static bool                          mRenderEnabledDebug3D{ true };
  static bool                          mUseLights{ true };
  static Render::CBufferLights         mDebugCBufferLights{};

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
    const int sizeofshaderlight_estimated{ 4 * ( 16 + 4 + 4 + 4 + 1 + 3 ) };
    const int sizeofshaderlight{ sizeof( Render::ShaderLight ) };
    const int light1Offset{ ( int )TAC_OFFSET_OF( Render::CBufferLights, lights[ 1 ] ) };
    const int light1OffsetReg{ light1Offset / 16 };
    const int light1OffsetAxis{ light1Offset % 16 };
    const int lightCountOffset{ ( int )TAC_OFFSET_OF( Render::CBufferLights, lightCount ) };
    const int lightCountOffsetReg{ lightCountOffset / 16 };
    const int lightCountOffsetAxis{ lightCountOffset % 16 };
    const bool check1{ sizeofshaderlight % 16 == 0 };
    const bool check2{ sizeofshaderlight == sizeofshaderlight_estimated };
    const bool check3{ light1OffsetReg == 8 };
    const bool check4{ light1OffsetAxis == 0 };
    const bool check5{ lightCountOffsetReg == 32 };
    const bool check6{ lightCountOffsetAxis == 0 };
    TAC_ASSERT( check1 );
    TAC_ASSERT( check2 );
    TAC_ASSERT( check3 );
    TAC_ASSERT( check4 );
    TAC_ASSERT( check5 );
    TAC_ASSERT( check6 );
  }

  static m4 GetProjMtx( const Camera* camera, const v2i viewSize )
  {
    const float aspectRatio{ ( float )viewSize.x / ( float )viewSize.y };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs{ renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspectRatio },
      .mFOVYRadians   { camera->mFovyrad },
    };
    const m4 proj{ m4::ProjPerspective( projParams ) };
    return proj;
  }

  static Render::DefaultCBufferPerFrame GetPerFrameBuf( const Camera* camera,
                                                        const v2i viewSize )
  {
    const Timestamp elapsedSeconds{ Timestep::GetElapsedTime() };
    const m4 view{ camera->View() };
    const m4 proj{ GetProjMtx( camera, viewSize ) };
    return Render::DefaultCBufferPerFrame
    {
      .mView        { view },
      .mProjection  { proj },
      .mFar         { camera->mFarPlane },
      .mNear        { camera->mNearPlane },
      .mGbufferSize { ( float )viewSize.x,  ( float )viewSize.y  },
      .mSecModTau   { ( float )Fmod( elapsedSeconds, 6.2831853 ) },
    };
  }

  static TerrainVertex GetTerrainVertex( const Terrain* terrain,
                                         const int r,
                                         const int c,
                                         const v2 uv )
  {
    return TerrainVertex
    {
      .mPos { terrain->GetGridVal( r, c ) },
      .mNor { terrain->GetGridValNormal( r, c ) },
      .mUV  { uv },
    };
  }

  static void Debug3DEachTri( Graphics* graphics )
  {
    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        Errors errors;
        Mesh* mesh{ ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                             model->mModelIndex,
                                                             GamePresentationGetVertexDeclarations(),
                                                             errors ) };
        if( !mesh )
          return;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          for( const SubMeshTriangle& tri : subMesh.mTris )
          {
            const v3 p0{ ( model->mEntity->mWorldTransform * v4( tri[ 0 ], 1 ) ).xyz() };
            const v3 p1{ ( model->mEntity->mWorldTransform * v4( tri[ 1 ], 1 ) ).xyz() };
            const v3 p2{ ( model->mEntity->mWorldTransform * v4( tri[ 2 ], 1 ) ).xyz() };
            mDrawData->DebugDraw3DTriangle( p0, p1, p2 );
          }
        }
      }
      Debug3DDrawData* mDrawData{ nullptr };
    } visitor{};
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

  static void RenderGameWorldAddDrawCall( Render::IContext* renderContext,
                                          const Light** lights,
                                          int lightCount,
                                          const Model* model,
                                          const Render::TextureHandle viewId,
                                          Errors& errors )
  {
    const Mesh* mesh{ LoadModel( model ) };
    if( !mesh )
      return;

    const Render::DefaultCBufferPerObject perObjectData
    {
      .World { model->mEntity->mWorldTransform },
      .Color { Render::PremultipliedAlpha::From_sRGB( model->mColorRGB ) },
    };

    FixedVector< Render::TextureHandle, Render::CBufferLights::TAC_MAX_SHADER_LIGHTS > shadowMaps;

    Render::CBufferLights cBufferLights;
    if( mUseLights )
    {
      for( int i{}; i < lightCount; ++i )
      {
        const Light* light{ lights[ i ] };
        const int shaderLightIndex{ (int)cBufferLights.lightCount };
        if( cBufferLights.TryAddLight( LightToShaderLight( light ) ) )
        {
          shadowMaps.push_back( light->mShadowMapDepth );
        }
      }
    }

    for( int i{}; i < shadowMaps.size(); ++i )
      mShaderGameShadowMaps->SetTextureAtIndex( i, shadowMaps[ i ] );

    mDebugCBufferLights = cBufferLights;

    mShaderGameLinearSampler->SetSampler( mSamplerLinear );
    mShaderGameShadowSampler->SetSampler( mSamplerPoint );

    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      const ShortFixedString groupName{
        ShortFixedString::Concat( model->mEntity->mName, " ", subMesh.mName ) };

      const Render::DrawArgs drawArgs
      {
        .mIndexCount { subMesh.mIndexCount },
        .mStartIndex { 0 },
      };

      TAC_RENDER_GROUP_BLOCK( renderContext, groupName );

      const Render::UpdateBufferParams updatePerObject
      {
        .mSrcBytes     { &perObjectData },
        .mSrcByteCount { sizeof( Render::DefaultCBufferPerObject ) },
      };
      TAC_CALL( renderContext->UpdateBuffer( Render::DefaultCBufferPerObject::sHandle,updatePerObject, errors  ) );

      renderContext->SetPipeline( mGamePipeline );
      renderContext->CommitShaderVariables();
      renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
      renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
      renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      renderContext->Draw( drawArgs );
    }
  }

  static void LoadTerrain( Terrain* terrain, Errors& errors )
  {
    if( terrain->mVertexBuffer.IsValid() && terrain->mIndexBuffer.IsValid() )
      return;

    if( terrain->mRowMajorGrid.empty() )
      return;


    typedef u32 TerrainIndex;

    Vector< TerrainVertex > vertexes;
    Vector< TerrainIndex > indexes;

    for( int iRow{ 1 }; iRow < terrain->mSideVertexCount; ++iRow )
    {
      for( int iCol{ 1 }; iCol < terrain->mSideVertexCount; ++iCol )
      {
        const int iVertexTL{ vertexes.size() };
        vertexes.push_back( GetTerrainVertex( terrain, iRow - 1, iCol - 1, { 0, 1 } ) );

        const int iVertexTR{ vertexes.size() };
        vertexes.push_back( GetTerrainVertex( terrain, iRow - 1, iCol, { 1, 1 } ) );

        const int iVertexBL{ vertexes.size() };
        vertexes.push_back( GetTerrainVertex( terrain, iRow, iCol - 1, { 0, 0 } ) );

        const int iVertexBR{ vertexes.size() };
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

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateBufferParams vtxBufParams
    {
      .mByteCount     { vertexes.size() * ( int )sizeof( TerrainVertex ) },
      .mBytes         { vertexes.data() },
      .mStride        { sizeof( TerrainVertex ) },
      .mUsage         { Render::Usage::Default },
      .mBinding       { Render::Binding::VertexBuffer },
      .mOptionalName  { "terrain-vtx-buf" },
    };
    TAC_CALL(
      terrain->mVertexBuffer = renderDevice->CreateBuffer( vtxBufParams, errors ) );

    const Render::CreateBufferParams idxBufParams
    {
      .mByteCount     { indexes.size() * (int)sizeof( TerrainIndex ) },
      .mBytes         { indexes.data() },
      .mStride        { sizeof( TerrainIndex ) },
      .mUsage         { Render::Usage::Default },
      .mBinding       { Render::Binding::IndexBuffer },
      .mGpuBufferFmt  { Render::TexFmt::kR32_uint },
      .mOptionalName  { "terrain-idx-buf" },
    };

    TAC_CALL(
      terrain->mIndexBuffer = renderDevice->CreateBuffer( idxBufParams, errors ) );

    terrain->mIndexCount = indexes.size();
  }

  static void CreateTerrainShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::ProgramParams programParams
    {
      .mFileStem   { "Terrain" },
      .mStackFrame { TAC_STACK_FRAME },
    };
    mTerrainShader = renderDevice->CreateProgram( programParams, errors );
  }

  static void Create3DShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::ProgramParams programParams
    {
      .mFileStem   { "GamePresentation" },
      .mStackFrame { TAC_STACK_FRAME },
    };
    m3DShader = renderDevice->CreateProgram( programParams, errors );
  }

  static void Create3DVertexFormat()
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( GameModelVtx, mPos ) },
    };

    const Render::VertexDeclaration norDecl
    {
      .mAttribute         { Render::Attribute::Normal },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( GameModelVtx, mNor ) },
    };

    m3DVertexFormatDecls.clear();
    m3DVertexFormatDecls.push_back( posDecl );
    m3DVertexFormatDecls.push_back( norDecl );
  }

  static void CreateTerrainVertexDecls()
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( TerrainVertex, mPos ) },
    };
    const Render::VertexDeclaration norDecl
    {
      .mAttribute         { Render::Attribute::Normal },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( TerrainVertex, mNor ) },
    };
    const Render::VertexDeclaration uvDecl
    {
      .mAttribute         { Render::Attribute::Texcoord },
      .mFormat            { Render::VertexAttributeFormat::GetVector2() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( TerrainVertex, mUV ) },
    };

    mTerrainVtxDecls.clear();
    mTerrainVtxDecls.push_back( posDecl );
    mTerrainVtxDecls.push_back( norDecl );
    mTerrainVtxDecls.push_back( uvDecl );
  }

  static Render::DepthState GetDepthState()
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }

  static Render::BlendState GetBlendState()
  {
    const Render::BlendState state
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::Zero },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add },
    };
    return state;
  }

  static Render::RasterizerState GetRasterizerState()
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::Back },
      .mFrontCounterClockwise { true },
      .mMultisample           { false },
    };
  }

  static void CreateLinearSampler()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateSamplerParams params
    {
      .mFilter { Render::Filter::Linear },
      .mName   { "game-linear-sampler" },
    };
    mSamplerLinear = renderDevice->CreateSampler( params );
  }

  static void CreatePointSampler()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateSamplerParams params
    {
      .mFilter { Render::Filter::Point},
      .mName   { "game-point-sampler" },
    };
    mSamplerPoint = renderDevice->CreateSampler( params );
  }

  static void CreateAnisoSampler()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateSamplerParams params
    {
      .mFilter { Render::Filter::Aniso},
      .mName   { "game-aniso-sampler" },
    };
    mSamplerAniso = renderDevice->CreateSampler( params );
  }

  static void RenderModels( Render::IContext* renderContext,
                            World* world,
                            const Camera* camera,
                            const v2i viewSize,
                            const Render::TextureHandle viewId,
                            Errors& errors )
  {
    if( !mRenderEnabledModel )
      return;

    const Render::DefaultCBufferPerFrame perFrameData{
      GetPerFrameBuf( camera, viewSize ) };

    const Render::UpdateBufferParams updateBufferParams
    {
      .mSrcBytes     { &perFrameData },
      .mSrcByteCount { sizeof( Render::DefaultCBufferPerFrame ) },
    };
    renderContext->UpdateBuffer( Render::DefaultCBufferPerFrame::sHandle,
                                 updateBufferParams,
                                 errors );

    Graphics* graphics{ GetGraphics( world ) };
    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        static struct : public LightVisitor
        {
          void operator()( Light* light ) override { mLights.push_back( light ); }
          Vector< const Light* > mLights;
        } sLightVisitor;

        Errors& errors{ *mErrors };
        sLightVisitor.mLights.resize( 0 );

        if( mUseLights )
          mGraphics->VisitLights( &sLightVisitor );

        sLightVisitor.mLights.resize(
          Min( sLightVisitor.mLights.size(), Render::CBufferLights::TAC_MAX_SHADER_LIGHTS ) );


        TAC_CALL( RenderGameWorldAddDrawCall( mRenderContext,
                                              sLightVisitor.mLights.data(),
                                              sLightVisitor.mLights.size(),
                                              model,
                                              mViewId,
                                              errors ) );
      }

      Render::IContext*     mRenderContext {};
      Render::TextureHandle mViewId        {};
      Graphics*             mGraphics      {};
      Errors*               mErrors        {};
    } myModelVisitor;
    myModelVisitor.mViewId = viewId;
    myModelVisitor.mGraphics = graphics;
    myModelVisitor.mRenderContext = renderContext;
    myModelVisitor.mErrors = &errors;

    TAC_RENDER_GROUP_BLOCK( renderContext, "Visit Models" );
    graphics->VisitModels( &myModelVisitor );
  }

  static void RenderTerrain( Render::IContext* renderContext,
                             World* world,
                             const Camera* camera,
                             const v2i viewSize,
                             const Render::TextureHandle viewId, 
                             Errors& errors )
  {
    if( !mRenderEnabledTerrain )
      return;

    const m4 worldmtx{  m4::Identity()  };
    const m4 view{ camera->View() };
    const m4 proj{ GetProjMtx( camera, viewSize ) };
    const TerrainConstBuf terrainConstBuf
    {
      .mWorld{ worldmtx },
      .mView{ view },
      .mProj{ proj },
    };

    const Render::UpdateBufferParams updateBufferParams
    {
      .mSrcBytes     { &terrainConstBuf },
      .mSrcByteCount { sizeof( TerrainConstBuf ) },
    };
    TAC_CALL( renderContext->UpdateBuffer( mTerrainConstBuf, updateBufferParams, errors ) );

    renderContext->SetPipeline( mTerrainPipeline );
    Physics* physics{ Physics::GetSystem( world ) };

    TAC_RENDER_GROUP_BLOCK( renderContext, "Visit Terrains" );
    for( Terrain* terrain : physics->mTerrains )
    {
      TAC_CALL( LoadTerrain( terrain, errors ) );

      if( !terrain->mVertexBuffer.IsValid() || !terrain->mIndexBuffer.IsValid() )
        continue;

      const Render::TextureHandle terrainTexture =
        TextureAssetManager::GetTexture( terrain->mGroundTexturePath, mGetTextureErrorsGround );
      mShaderTerrainHeightmap->SetTexture( terrainTexture );

      const Render::TextureHandle noiseTexture =
        TextureAssetManager::GetTexture( terrain->mNoiseTexturePath, mGetTextureErrorsNoise );
      mShaderTerrainNoise->SetTexture( noiseTexture );

      const Render::DefaultCBufferPerObject cbuf{};
      const Render::DrawArgs drawArgs
      {
        .mIndexCount { terrain->mIndexCount },
        .mStartIndex { 0 },
      };
 
      renderContext->CommitShaderVariables();
      renderContext->SetIndexBuffer( terrain->mIndexBuffer );
      renderContext->SetVertexBuffer( terrain->mVertexBuffer );
      renderContext->Draw( drawArgs );
    }
  }


  static void RenderSkybox( Render::IContext* renderContext,
                            World* world,
                            const Camera* camera,
                            const v2i viewSize,
                            const Render::TextureHandle viewId,
                            Errors& errors )
  {

    if( !mRenderEnabledSkybox )
      return;

    Graphics* graphics{ GetGraphics( world ) };
    struct : public SkyboxVisitor
    {
      void operator()( Skybox* skybox ) override
      {
        const SkyboxRenderParams skyboxRenderParams
        {
          .mCamera    { mCamera },
          .mViewSize  { mViewSize },
          .mViewId    { mViewId },
          .mSkyboxDir { skybox->mSkyboxDir }
        };
        SkyboxPresentationRender( skyboxRenderParams, *mErrors );
      }
      v2i                    mViewSize {};
      Render::TextureHandle  mViewId   {};
      const Camera*          mCamera   {};
      Errors*                mErrors   {};
    } mySkyboxVisitor;
    mySkyboxVisitor.mViewSize = viewSize;
    mySkyboxVisitor.mViewId = viewId;
    mySkyboxVisitor.mCamera = camera;
    mySkyboxVisitor.mErrors = &errors;

    TAC_RENDER_GROUP_BLOCK( renderContext, "Visit Skyboxes" );
    graphics->VisitSkyboxes( &mySkyboxVisitor );
  }

  static void DebugImguiCBufferLight( int iLight )
  {
    if( !ImGuiCollapsingHeader( ShortFixedString::Concat( "Light ", ToString( iLight ) ) ) )
      return;

    Render::ShaderLight* shaderLight { &mDebugCBufferLights.lights[ iLight ] };
    String rowsStrs[ 4 ];
    for( int r {  }; r < 4; ++r )
      for( int c {  }; c < 4; ++c )
        rowsStrs[ r ] += ToString( shaderLight->mWorldToClip( r, c ) );

    const Render::ShaderFlags::Info* lightTypeInfo { Render::GetShaderLightFlagType() };
    const Render::ShaderFlags::Info* castsShadowsInfo { Render::GetShaderLightFlagCastsShadows() };

    const Light::Type lightType { ( Light::Type )lightTypeInfo->Extract( shaderLight->mFlags ) };
    const bool castsShadows { ( bool )castsShadowsInfo->Extract( shaderLight->mFlags ) };

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
    ImGuiImage( -1, v2( 1, 1 ) * 50, v4( shaderLight->mColorRadiance.xyz(), 1.0f ) );
    ImGuiText( ShortFixedString::Concat( "Light type: ",
               LightTypeToString( lightType ),
               "(",
               ToString( ( int )lightType ),
               ")" ) );
    ImGuiText( String() + "casts shadows: " + ( castsShadows ? "true" : "false" ) );
  }

  static void DebugImguiCBufferLights()
  {
    if( !ImGuiCollapsingHeader( "CBufferLights" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    ImGuiText( String() + "light count " + ToString( mDebugCBufferLights.lightCount ) );
    ImGuiText( String() + "text number " + ToString( mDebugCBufferLights.testNumber ) );

    const int n { ( int )mDebugCBufferLights.lightCount };
    for( int iLight {}; iLight < n; iLight++ )
      DebugImguiCBufferLight( iLight );
  }

  static void DebugImgui3DTris( Graphics* graphics )
  {
    if( !mRenderEnabledDebug3D )
      return;

    static bool debugEachTri;
    ImGuiCheckbox( "debug each tri", &debugEachTri );

    if( debugEachTri )
      Debug3DEachTri( graphics );
  }

}

const Tac::Mesh* Tac::GamePresentationGetModelMesh( const Model* model )
{
  return LoadModel( model );
}

void        Tac::GamePresentationInit( Errors& errors )
{
  CheckShaderPadding();

  TAC_CALL( Create3DShader( errors ) );

  TAC_CALL( CreateTerrainShader( errors ) );

  Create3DVertexFormat();
  CreateTerrainVertexDecls();

  const Render::BlendState blendState{ GetBlendState() };
  const Render::DepthState depthState{ GetDepthState() };
  const Render::RasterizerState rasterizerState{ GetRasterizerState() };

  CreatePointSampler();
  CreateLinearSampler();
  CreateAnisoSampler();

  const Render::PipelineParams terrainPipelineParams
  {
    .mProgram           { mTerrainShader},
    .mBlendState        { blendState},
    .mDepthState        { depthState},
    .mRasterizerState   { rasterizerState },
    .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
    .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
    .mVtxDecls          { mTerrainVtxDecls },
    .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
    .mName              { "terrain-pso" },
  };

  const Render::PipelineParams gamePipelineParams
  {
    .mProgram           { m3DShader},
    .mBlendState        { blendState},
    .mDepthState        { depthState},
    .mRasterizerState   { rasterizerState },
    .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
    .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
    .mVtxDecls          { m3DVertexFormatDecls },
    .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
    .mName              { "game-pso" },
  };

  Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
  TAC_CALL( mTerrainPipeline = renderDevice->CreatePipeline( terrainPipelineParams, errors ) );
  TAC_CALL( mGamePipeline = renderDevice->CreatePipeline( gamePipelineParams, errors ) );
  
  mShaderTerrainHeightmap = renderDevice->GetShaderVariable( mTerrainPipeline, "terrainTexture" );
  mShaderTerrainNoise = renderDevice->GetShaderVariable( mTerrainPipeline, "noiseTexture" );
  mShaderTerrainSampler = renderDevice->GetShaderVariable( mTerrainPipeline, "linearSampler" );
  mShaderTerrainSampler->SetSampler( mSamplerAniso );

  const Render::CreateBufferParams terrainConstBufParams
  {
    .mByteCount     { sizeof( TerrainConstBuf ) },
    .mUsage         { Render::Usage::Dynamic },
    .mBinding       { Render::Binding::ConstantBuffer },
    .mOptionalName  { "terrain-const-buf" },
  };
  TAC_CALL( mTerrainConstBuf = renderDevice->CreateBuffer( terrainConstBufParams, errors ) );
  mShaderTerrainConstBuf = renderDevice->GetShaderVariable( mTerrainPipeline, "terrainConstBuf" );
  mShaderTerrainConstBuf->SetBuffer( mTerrainConstBuf );

  mShaderGameShadowMaps = renderDevice->GetShaderVariable( mGamePipeline, "shadowMaps" );
  mShaderGameLinearSampler = renderDevice->GetShaderVariable( mGamePipeline, "linearSampler" );
  mShaderGameLinearSampler->SetSampler( mSamplerLinear );
  mShaderGameShadowSampler = renderDevice->GetShaderVariable( mGamePipeline, "shadowMapSampler" );
  mShaderGameShadowSampler->SetSampler( mSamplerPoint );

  mShaderGamePerObject = renderDevice->GetShaderVariable( mGamePipeline, "CBufferPerObject" );
  mShaderGamePerObject->SetBuffer( Render::DefaultCBufferPerObject::sHandle );
}

void        Tac::GamePresentationUninit()
{
  Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
  renderDevice->DestroyProgram( m3DShader );
  renderDevice->DestroyPipeline( mTerrainPipeline );
  renderDevice->DestroyPipeline( mGamePipeline );
  renderDevice->DestroySampler( mSamplerAniso );
  renderDevice->DestroySampler( mSamplerPoint );
  renderDevice->DestroySampler( mSamplerLinear );
  renderDevice->DestroyBuffer( mTerrainConstBuf );
}


void        Tac::GamePresentationRender( World* world,
                                         const Camera* camera,
                                         const v2i viewSize,
                                         const Render::TextureHandle dstColorTex,
                                         Errors& errors )
{
  Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
  Render::IContext::Scope renderContextScope{ renderDevice->CreateRenderContext( errors ) };
  Render::IContext* renderContext{ renderContextScope.GetContext() };

  TAC_RENDER_GROUP_BLOCK( renderContext, "GamePresentationRender" );

  ShadowPresentationRender( world );

  TAC_CALL( RenderModels( renderContext,
                          world,
                          camera,
                          viewSize,
                          dstColorTex,
                          errors ) );

  TAC_CALL( RenderTerrain( renderContext,
                           world,
                           camera,
                           viewSize,
                           dstColorTex,
                           errors ) );

  // Skybox should be last to reduce pixel shader invocations
  TAC_CALL( RenderSkybox( renderContext,
                          world,
                          camera,
                          viewSize,
                          dstColorTex,
                          errors ) );

  if( mRenderEnabledDebug3D )
  {
    TAC_CALL( world->mDebug3DDrawData->DebugDraw3DToTexture( renderContext,
                                                             dstColorTex,
                                                             camera,
                                                             viewSize,
                                                             errors ) );
  }

  TAC_CALL( renderContext->Execute( errors ) );
}

//Render::DepthStateHandle      GamePresentationGetDepthState()           { return mDepthState; }
//Render::BlendStateHandle      GamePresentationGetBlendState()           { return mBlendState; }
//Render::RasterizerStateHandle GamePresentationGetRasterizerState()      { return mRasterizerState; }
//Render::SamplerStateHandle    GamePresentationGetSamplerState()         { return mSamplerStateAniso; }
//Render::VertexDeclarations    GamePresentationGetVertexDeclarations()   { return m3DVertexFormatDecls; }
//Render::VertexFormatHandle    GamePresentationGetVertexFormat()         { return m3DVertexFormat; }


void Tac::GamePresentationDebugImGui( Graphics* graphics )
{
  if( !ImGuiCollapsingHeader( "Game Presentation" ) )
    return;

  TAC_IMGUI_INDENT_BLOCK;
  ImGuiCheckbox( "Game Presentation Enabled Model", &mRenderEnabledModel );
  ImGuiCheckbox( "Game Presentation Enabled Skybox", &mRenderEnabledSkybox );
  ImGuiCheckbox( "Game Presentation Enabled Terrain", &mRenderEnabledTerrain );
  ImGuiCheckbox( "Game Presentation Enabled Debug3D", &mRenderEnabledDebug3D );

  ImGuiCheckbox( "Game Presentation use lights", &mUseLights );
  if( mUseLights )
    DebugImguiCBufferLights();

  DebugImgui3DTris( graphics );
}

#endif
