#include "tac_mesh_presentation.h" // self-inc

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
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/presentation/shadow/tac_shadow_presentation.h"
#include "tac-ecs/presentation/skybox/tac_skybox_presentation.h"
#include "tac-ecs/presentation/terrain/tac_terrain_presentation.h"
#include "tac-ecs/presentation/voxel/tac_voxel_gi_presentation.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/terrain/tac_terrain.h"


#if TAC_MESH_PRESENTATION_ENABLED()

namespace Tac
{

  struct MeshPerFrameBuf
  {
    m4 mView    {};
    m4 mProj    {};
    v4 mAmbient {};
  };

  struct MeshPerObjBuf
  {
    m4 mWorld {};
    v4 mColor {};
  };

  struct MeshModelVtx
  {
    v3 mPos {};
    v3 mNor {};
  };

  struct Samplers
  {
    Render::SamplerHandle GetSampler( Render::Filter filter )
    {
      return mSamplerHandles[ (int)filter ];
    }

    void Init()
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

      for( int i{}; i < ( int )Render::Filter::Count; ++i )
      {
        const Render::Filter filter{( Render::Filter )i};
        const char* name{ GetName( filter ) };
        const Render::CreateSamplerParams params
        {
          .mFilter { filter },
          .mName   { name },
        };
        mSamplerHandles[ i ] = renderDevice->CreateSampler( params );
      }
    }

    void Uninit()
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      for( Render::SamplerHandle& samplerHandle : mSamplerHandles )
      {
        renderDevice->DestroySampler( samplerHandle );
        samplerHandle = {};
      }
    }

  private:

    const char* GetName( Render::Filter filter )
    {
      switch ( filter )
      {
      case Render::Filter::Linear: return "linear";
      case Render::Filter::Point: return "linear";
      case Render::Filter::Aniso: return "aniso";
      default: TAC_ASSERT_INVALID_CASE( filter ); return "";
      }
    }

    Render::SamplerHandle mSamplerHandles[ ( int )Render::Filter::Count ];
  };


  static Render::BufferHandle          mMeshPerFrameBuf;
  static Render::BufferHandle          mMeshPerObjBuf;
  static Render::TextureHandle         s1x1White;
  static Render::ProgramHandle         m3DShader;
  static Render::PipelineHandle        mMeshPipeline;
  static Samplers                      sSamplers;
  static Render::VertexDeclarations    m3DVertexFormatDecls;
  static Render::IShaderVar*           mShaderMeshShadowMaps;
  static Render::IShaderVar*           mShaderMeshLights;
  static Render::IShaderVar*           mShaderMeshShadowSampler;
  static Render::IShaderVar*           mShaderMeshPerFrame;
  static Render::IShaderVar*           mShaderMeshPerObject;
  static Errors                        mGetTextureErrorsGround;
  static Errors                        mGetTextureErrorsNoise;
  static bool                          mRenderEnabledModel      { false };
  static bool                          mUseLights               { true };
  static Render::CBufferLights         mDebugCBufferLights      {};
  static bool                          sInitialized             {};
  static v4                            sAmbient                 { 0.4f, 0.3f, 0.2f, 1.0f };
  static bool                          sUseAmbient              { true };


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

  static MeshPerFrameBuf GetPerFrameBuf( const Camera* camera,
                                         const v2i viewSize )
  {
    const m4 view{ camera->View() };
    const m4 proj{ GetProjMtx( camera, viewSize ) };
    return MeshPerFrameBuf
    {
      .mView    { view },
      .mProj    { proj },
      .mAmbient { sUseAmbient ? sAmbient : v4{ 0, 0, 0, 0 } },
    };
  }

  static Mesh* LoadModel( const Model* model, Errors& errors )
  {
    const ModelAssetManager::Params meshParams
    {
      .mPath        { model->mModelPath },
      .mModelIndex  { model->mModelIndex },
      .mOptVtxDecls { m3DVertexFormatDecls },
    };
    return ModelAssetManager::GetMesh( meshParams, errors );
  }

  static void UpdatePerFrameCBuf( const Camera* camera,
                                  const v2i viewSize,
                                  Render::IContext* renderContext,
                                  Errors& errors )
  {
    const MeshPerFrameBuf perFrameData{ GetPerFrameBuf( camera, viewSize ) };

    const Render::UpdateBufferParams updateBufferParams
    {
      .mSrcBytes     { &perFrameData },
      .mSrcByteCount { sizeof( MeshPerFrameBuf ) },
    };
    TAC_CALL( renderContext->UpdateBuffer( mMeshPerFrameBuf, updateBufferParams, errors ) );
  }

  static void UpdatePerObjectCBuf( const Model* model,
                                   const Mesh* ,
                                   Render::IContext* renderContext,
                                   Errors& errors )
  {
    v4 color{ 1, 1, 1, 1 };

    const Render::DefaultCBufferPerObject perObjectData
    {
      .World { model->mEntity->mWorldTransform },
      .Color { Render::PremultipliedAlpha::From_sRGB_linearAlpha( color ) },
    };

    const Render::UpdateBufferParams updatePerObject
    {
      .mSrcBytes     { &perObjectData },
      .mSrcByteCount { sizeof( MeshPerObjBuf ) },
    };
    TAC_CALL( renderContext->UpdateBuffer( mMeshPerObjBuf, updatePerObject, errors ) );
  }

  static void RenderMeshWorldAddDrawCall( Render::IContext* renderContext,
                                          Span< const Light* > lights,
                                          const Model* model,
                                          const Render::TextureHandle viewId,
                                          Errors& errors )
  {
    TAC_UNUSED_PARAMETER( viewId );
    TAC_CALL( const Mesh* mesh{ LoadModel( model, errors ) } );
    if( !mesh )
      return;

    FixedVector< Render::TextureHandle, Render::CBufferLights::TAC_MAX_SHADER_LIGHTS > shadowMaps;
    Render::CBufferLights cBufferLights;

    // populate `shadowMaps` and `cBufferLights`
    if( mUseLights )
    {
      const int nLights{ lights.size() };
      for( int i{}; i < nLights; ++i )
      {
        const Light* light{ lights[ i ] };
        //const int shaderLightIndex{ ( int )cBufferLights.lightCount };
        if( cBufferLights.TryAddLight( LightToShaderLight( light ) ) )
        {
          shadowMaps.push_back( light->mShadowMapDepth );
        }
      }
    }

    mShaderMeshLights->SetResource( Render::CBufferLights::sHandle );
    const Render::UpdateBufferParams updateLights
    {
      .mSrcBytes     { &cBufferLights },
      .mSrcByteCount { sizeof( Render::CBufferLights ) },
    };
    TAC_CALL( renderContext->UpdateBuffer( Render::CBufferLights::sHandle,
                                           updateLights,
                                           errors ) );

    const int nShadowMaps{ shadowMaps.size() };
    for( int i{}; i < nShadowMaps; ++i )
      mShaderMeshShadowMaps->SetResourceAtIndex( shadowMaps[ i ], i );

    for( int i{ nShadowMaps }; i < Render::CBufferLights::TAC_MAX_SHADER_LIGHTS; ++i )
    {
      // hack, currently every element needs to be bound
      mShaderMeshShadowMaps->SetResourceAtIndex( s1x1White, i );
    }

    mDebugCBufferLights = cBufferLights;

    //mShaderMeshLinearSampler->SetSampler( mSamplerLinear );
    mShaderMeshShadowSampler->SetResource( sSamplers.GetSampler( Render::Filter::Point ) );

    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      const ShortFixedString groupName{
        ShortFixedString::Concat( model->mEntity->mName, " ", subMesh.mName ) };

      const Render::DrawArgs drawArgs
      {
        .mVertexCount { subMesh.mVertexCount },
        .mIndexCount  { subMesh.mIndexCount },
      };

      TAC_RENDER_GROUP_BLOCK( renderContext, groupName );

      TAC_CALL( UpdatePerObjectCBuf( model, mesh, renderContext, errors ) );

      renderContext->SetPipeline( mMeshPipeline );
      renderContext->CommitShaderVariables();
      renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
      renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
      renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      renderContext->Draw( drawArgs );
    }
  }

  static void Create3DShader( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::ProgramParams programParams
    {
      .mInputs     { "MeshPresentation" },
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
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( MeshModelVtx, mPos ) },
    };

    const Render::VertexDeclaration norDecl
    {
      .mAttribute         { Render::Attribute::Normal },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( MeshModelVtx, mNor ) },
    };

    m3DVertexFormatDecls.clear();
    m3DVertexFormatDecls.push_back( posDecl );
    m3DVertexFormatDecls.push_back( norDecl );
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


  static void RenderModels( Render::IContext* renderContext,
                            const World* world,
                            const Camera* camera,
                            const v2i viewSize,
                            const Render::TextureHandle viewId,
                            Errors& errors )
  {
    if( !mRenderEnabledModel )
      return;


    TAC_CALL( UpdatePerFrameCBuf( camera, viewSize, renderContext, errors ) );

    const Graphics* graphics{ Graphics::From( world ) };
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

        Span< const Light* > lightSpan( sLightVisitor.mLights.data(),
                                        sLightVisitor.mLights.size() );

        TAC_CALL( RenderMeshWorldAddDrawCall( mRenderContext,
                                              lightSpan,
                                              model,
                                              mViewId,
                                              errors ) );
      }

      Render::IContext* mRenderContext{};
      Render::TextureHandle mViewId{};
      const Graphics* mGraphics{};
      Errors* mErrors{};
    } myModelVisitor;
    myModelVisitor.mViewId = viewId;
    myModelVisitor.mGraphics = graphics;
    myModelVisitor.mRenderContext = renderContext;
    myModelVisitor.mErrors = &errors;

    TAC_RENDER_GROUP_BLOCK( renderContext, "Visit Models" );
    graphics->VisitModels( &myModelVisitor );
  }




  static void DebugImguiCBufferLight( int iLight )
  {
    if( !ImGuiCollapsingHeader( ShortFixedString::Concat( "Light ", ToString( iLight ) ) ) )
      return;

    Render::ShaderLight* shaderLight{ &mDebugCBufferLights.lights[ iLight ] };
    String rowsStrs[ 4 ];
    for( int r{  }; r < 4; ++r )
      for( int c{  }; c < 4; ++c )
        rowsStrs[ r ] += ToString( shaderLight->mWorldToClip( r, c ) );

    const Render::ShaderFlags::Info* lightTypeInfo{ Render::GetShaderLightFlagType() };
    const Render::ShaderFlags::Info* castsShadowsInfo{ Render::GetShaderLightFlagCastsShadows() };

    const Light::Type lightType{ ( Light::Type )lightTypeInfo->Extract( shaderLight->mFlags ) };
    const bool castsShadows{ ( bool )castsShadowsInfo->Extract( shaderLight->mFlags ) };

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

    const int n{ ( int )mDebugCBufferLights.lightCount };
    for( int iLight{}; iLight < n; iLight++ )
      DebugImguiCBufferLight( iLight );
  }

  static void Create1x1White(Errors& errors)
  {
    char bytes[ 4 ]{ 1, 1, 1, 1 };
    const Render::CreateTextureParams::Subresource subRsc
    {
      .mBytes { bytes },
      .mPitch { 4 },
    };

    const Render::Image img
    {
      .mWidth   { 1 },
      .mHeight  { 1 },
      .mFormat  { Render::TexFmt::kRGBA8_unorm },
    };

    const Render::CreateTextureParams createTextureParams
    {
      .mImage        { img },
      .mMipCount     { 1 },
      .mSubresources { subRsc },
      .mBinding      { Render::Binding::ShaderResource },
      .mOptionalName { "unbound_shadowmap"},
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( s1x1White = renderDevice->CreateTexture( createTextureParams, errors ) );
  }

  static void CreatePerMesh( Errors& errors )
  {
    const Render::CreateBufferParams meshPerFrameParams
    {
      .mByteCount     { sizeof( MeshPerFrameBuf ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "mesh-per-frame-cbuf" },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( mMeshPerFrameBuf = renderDevice->CreateBuffer( meshPerFrameParams, errors ) );
  }

  static void CreatePerObj( Errors& errors )
  {
    const Render::CreateBufferParams meshPerObjParams
    {
      .mByteCount     { sizeof( MeshPerObjBuf ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "mesh-per-obj-cbuf" },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( mMeshPerObjBuf = renderDevice->CreateBuffer( meshPerObjParams, errors ) );
  }

  // -----------------------------------------------------------------------------------------------

  const Tac::Mesh* MeshPresentation::GetModelMesh( const Model* model, Errors& errors )
  {
    return LoadModel( model, errors );
  }

  void             MeshPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;

    CheckShaderPadding();

    TAC_CALL( Create3DShader( errors ) );


    Create3DVertexFormat();

    const Render::BlendState blendState{ GetBlendState() };
    const Render::DepthState depthState{ GetDepthState() };
    const Render::RasterizerState rasterizerState{ GetRasterizerState() };

    sSamplers.Init();

    const Render::PipelineParams meshPipelineParams
    {
      .mProgram           { m3DShader },
      .mBlendState        { blendState },
      .mDepthState        { depthState },
      .mRasterizerState   { rasterizerState },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mVtxDecls          { m3DVertexFormatDecls },
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "mesh-pso" },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( mMeshPipeline = renderDevice->CreatePipeline( meshPipelineParams, errors ) );

    mShaderMeshLights = renderDevice->GetShaderVariable( mMeshPipeline, "CBufferLights" );

    mShaderMeshShadowMaps = renderDevice->GetShaderVariable( mMeshPipeline, "shadowMaps" );
    mShaderMeshShadowSampler = renderDevice->GetShaderVariable( mMeshPipeline, "shadowMapSampler" );
    mShaderMeshShadowSampler->SetResource( sSamplers.GetSampler( Render::Filter::Point ) );

    TAC_CALL( CreatePerMesh( errors ) );
    TAC_CALL( CreatePerObj( errors ) );

    mShaderMeshPerObject = renderDevice->GetShaderVariable( mMeshPipeline, "sPerObj" );
    mShaderMeshPerObject->SetResource( mMeshPerObjBuf );

    mShaderMeshPerFrame = renderDevice->GetShaderVariable( mMeshPipeline, "sPerFrame" );
    mShaderMeshPerFrame->SetResource( mMeshPerFrameBuf );

    TAC_CALL( Create1x1White( errors ) );

    TAC_CALL( SkyboxPresentation::Init( errors ) );
    TAC_CALL( ShadowPresentation::Init( errors ) );

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    TAC_CALL( VoxelGIPresentationInit( errors ) );
#endif
    sInitialized = true;
  }

  void             MeshPresentation::Uninit()
  {
    if( sInitialized )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      renderDevice->DestroyProgram( m3DShader );
      renderDevice->DestroyPipeline( mMeshPipeline );
      sSamplers.Uninit();
      SkyboxPresentation::Uninit();
      ShadowPresentation::Uninit();
#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
      VoxelGIPresentationUninit();
#endif
      sInitialized = false;
    }

  }

  void             MeshPresentation::Render( Render::IContext* renderContext,
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

    renderContext->DebugEventBegin( "MeshPresentationRender" );
    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    renderContext->SetRenderTargets( renderTargets );

#if 0
    TAC_CALL( ShadowPresentationRender( world, errors ) );
#endif

    TAC_CALL( RenderModels( renderContext,
                            world,
                            camera,
                            viewSize,
                            dstColorTex,
                            errors ) );

#if 0
#if TAC_TERRAIN_PRESENTATION_ENABLED()
    TAC_CALL( RenderTerrain( renderContext,
                             world,
                             camera,
                             viewSize,
                             dstColorTex,
                             errors ) );
#endif
#endif


#if 0
#if TAC_SKYBOX_PRESENTATION_ENABLED()
    // Skybox should be last to reduce pixel shader invocations
    TAC_CALL( SkyboxPresentationRender( renderContext,
                                        world,
                                        camera,
                                        viewSize,
                                        dstColorTex,
                                        errors ) );
#endif
#endif

    renderContext->DebugEventEnd();
  }

  void             MeshPresentation::DebugImGui()
  {
    if( !ImGuiCollapsingHeader( "Mesh Presentation" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
    ImGuiCheckbox( "Render Models", &mRenderEnabledModel );

    ImGuiCheckbox( "Use lights", &mUseLights );

    ImGuiCheckbox( "Use Ambient", &sUseAmbient );
    if( ImGuiDragFloat3( "ambient color", sAmbient.data() ) )
    {
      sAmbient.x = Saturate( sAmbient.x );
      sAmbient.y = Saturate( sAmbient.y );
      sAmbient.z = Saturate( sAmbient.z );
      sAmbient.w = Saturate( sAmbient.w );
    }

    if( mUseLights )
      DebugImguiCBufferLights();

  }

  // -----------------------------------------------------------------------------------------------

}
#endif
