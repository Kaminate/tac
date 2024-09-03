#include "tac_terrain_presentation.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/presentation/shadow/tac_shadow_presentation.h"
#include "tac-ecs/presentation/skybox/tac_skybox_presentation.h"
#include "tac-ecs/terrain/tac_terrain.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/tac_ints.h"

#if TAC_TERRAIN_PRESENTATION_ENABLED()

namespace Tac
{
  struct TerrainConstBuf
  {
    m4 mWorld;
    m4 mView;
    m4 mProj;
  };

  struct GamePerFrameBuf
  {
    m4 mView;
    m4 mProj;
    v4 mAmbient{};
  };

  struct GamePerObjBuf
  {
    m4 mWorld;
    v4 mColor;
  };

  struct TerrainVertex
  {
    v3 mPos;
    v3 mNor;
    v2 mUV;
  };

  static Render::BufferHandle          mTerrainConstBuf;
  static Render::ProgramHandle         mTerrainShader;
  static Render::PipelineHandle        mTerrainPipeline;
  static Render::SamplerHandle         mSamplerPoint;
  static Render::SamplerHandle         mSamplerLinear;
  static Render::SamplerHandle         mSamplerAniso;
  static Render::VertexDeclarations    mTerrainVtxDecls;
  static Render::IShaderVar*           mShaderTerrainHeightmap;
  static Render::IShaderVar*           mShaderTerrainNoise;
  static Render::IShaderVar*           mShaderTerrainSampler;
  static Render::IShaderVar*           mShaderTerrainConstBuf;
  static Errors                        mGetTextureErrorsGround;
  static Errors                        mGetTextureErrorsNoise;
  static bool                          mRenderEnabledTerrain{ true };


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
      .mInputs     { "Terrain" },
      .mStackFrame { TAC_STACK_FRAME },
    };
    mTerrainShader = renderDevice->CreateProgram( programParams, errors );
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
      .mMultisample           {},
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


  static void RenderTerrain( Render::IContext* renderContext,
                             const World* world,
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
      .mWorld { worldmtx },
      .mView  { view },
      .mProj  { proj },
    };

    const Render::UpdateBufferParams updateBufferParams
    {
      .mSrcBytes     { &terrainConstBuf },
      .mSrcByteCount { sizeof( TerrainConstBuf ) },
    };
    TAC_CALL( renderContext->UpdateBuffer( mTerrainConstBuf, updateBufferParams, errors ) );

    renderContext->SetPipeline( mTerrainPipeline );
    const Physics* physics{ Physics::GetSystem( world ) };

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
        .mStartIndex {},
      };
 
      renderContext->CommitShaderVariables();
      renderContext->SetIndexBuffer( terrain->mIndexBuffer );
      renderContext->SetVertexBuffer( terrain->mVertexBuffer );
      renderContext->Draw( drawArgs );
    }
  }


  // -----------------------------------------------------------------------------------------------

  void        TerrainPresentation::Init( Errors& errors )
  {
    TAC_CALL( CreateTerrainShader( errors ) );

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


    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( mTerrainPipeline = renderDevice->CreatePipeline( terrainPipelineParams, errors ) );
    
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
  }

  void        TerrainPresentation::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyPipeline( mTerrainPipeline );
    renderDevice->DestroySampler( mSamplerAniso );
    renderDevice->DestroySampler( mSamplerPoint );
    renderDevice->DestroySampler( mSamplerLinear );
    renderDevice->DestroyBuffer( mTerrainConstBuf );
  }


  void        TerrainPresentation::Render( Render::IContext* renderContext,
                                           const World* world,
                                           const Camera* camera,
                                           const v2i viewSize,
                                           const Render::TextureHandle dstColorTex,
                                           const Render::TextureHandle dstDepthTex,
                                           Errors& errors )
  {
    const Render::Targets renderTargets
    {
      .mColors { dstColorTex },
      .mDepth  { dstDepthTex },
    };


    renderContext->DebugEventBegin( "TerrainPresentationRender" );
    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    renderContext->SetRenderTargets( renderTargets );


    TAC_CALL( RenderTerrain( renderContext,
                             world,
                             camera,
                             viewSize,
                             dstColorTex,
                             errors ) );



    renderContext->DebugEventEnd();
  }

  void        TerrainPresentation::DebugImGui()
  {
    if( !ImGuiCollapsingHeader( "Terrain Presentation" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
    ImGuiCheckbox( "Terrain Enabled", &mRenderEnabledTerrain );
  }

  // -----------------------------------------------------------------------------------------------

} // namespace Tac

#endif
