#include "tac_skybox_presentation.h" // self-inc

#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"

#if TAC_SKYBOX_PRESENTATION_ENABLED()

namespace Tac
{
  static Render::ProgramHandle         mShader;
  static Render::SamplerHandle         mSamplerState;
  static Render::VertexDeclarations    mVertexDecls;
  static Render::PipelineHandle        sPipeline;
  static Render::BufferHandle          sConstantBuffer;
  static Render::IShaderVar*           sShaderConstantBuffer;
  static Render::IShaderVar*           sShaderCubemap;
  static Render::IShaderVar*           sShaderSampler;
  static bool                          sInitialized;
  static bool                          mRenderEnabledSkybox{ true };

  struct SkyboxConstantBuffer
  {
    m4 mView;
    m4 mProj;
  };

  struct SkyboxRenderParams
  {
    const Camera*         mCamera;
    v2i                   mViewSize;
    Render::TextureHandle mViewId;
    AssetPathStringView   mSkyboxDir;
  };

  static void RenderSkybox( Render::IContext* renderContext,
                            const World* world,
                            const Camera* camera,
                            const v2i viewSize,
                            const Render::TextureHandle viewId,
                            Errors& errors )
  {


  }

  static Skybox* GetSkybox( const World* world )
  {
    const Graphics* graphics{ GetGraphics( world ) };
    struct : public SkyboxVisitor
    {
      void operator()( Skybox* skybox ) override
      {
        mSkybox = skybox;
      }
      Skybox* mSkybox{};
    } mySkyboxVisitor;
    graphics->VisitSkyboxes( &mySkyboxVisitor );
    return mySkyboxVisitor.mSkybox;
  }

  // ---------------

  void SkyboxPresentation::Uninit()
  {
    if( sInitialized )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      renderDevice->DestroyProgram( mShader );
      //Render::DestroyVertexFormat( mVertexFormat, TAC_STACK_FRAME );
      sInitialized = false;
    }
  }

  void SkyboxPresentation::Init( Errors& errors )
  {
    if( sInitialized )
      return;


    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mFileStem { "Skybox" },
      .mStackFrame { TAC_STACK_FRAME },
    };
    mShader = renderDevice->CreateProgram( programParams, errors );

    const Render::VertexAttributeFormat posFmt{ Render::VertexAttributeFormat::GetVector3() };
    const Render::VertexDeclaration vtxDecl
    {
      .mAttribute { Render::Attribute::Position },
      .mFormat    { posFmt },
    };

    mVertexDecls.clear();
    mVertexDecls.push_back( vtxDecl );


    const Render::BlendState blendState
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::Zero },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add },
    };

    const Render::DepthState depthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::LessOrEqual },
    };

    const Render::RasterizerState rasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mMultisample           {},
    };

    const Render::CreateSamplerParams createSkyboxSampler
    {
      .mFilter { Render::Filter::Linear },
      .mName { "skybox-sampler" },
    };
    mSamplerState = renderDevice->CreateSampler( createSkyboxSampler );

    const Render::PipelineParams pipelineParams
    {
      .mProgram           { mShader },
      .mBlendState        { blendState },
      .mDepthState        { depthState },
      .mRasterizerState   { rasterizerState },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mVtxDecls          { mVertexDecls },
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "skybox_pipeline" },
    };
    TAC_CALL( sPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    sShaderCubemap = renderDevice->GetShaderVariable( sPipeline, "cubemap" );
    sShaderConstantBuffer = renderDevice->GetShaderVariable( sPipeline, "cBuf" );
    sShaderSampler = renderDevice->GetShaderVariable( sPipeline, "linearSampler" );
    sInitialized = true;
  }

  void SkyboxPresentation::Render( Render::IContext* renderContext,
                                      const World* world,
                                      const Camera* camera,
                                      const v2i viewSize,
                                      const Render::TextureHandle viewId,
                                      Errors& errors )
  {
    if( !mRenderEnabledSkybox )
      return;

    Skybox* skybox{ GetSkybox( world ) };
    if( !skybox )
      return;

    const AssetPathStringView skyboxDir{ skybox->mSkyboxDir };

    /*TAC_PROFILE_BLOCK*/;
    const AssetPathStringView defaultSkybox{ "assets/skybox/daylight" };
    const AssetPathStringView skyboxDirToUse{ skyboxDir.empty() ? defaultSkybox : skyboxDir };
    TAC_CALL( const Render::TextureHandle cubemap{
      TextureAssetManager::GetTextureCube( skyboxDirToUse, errors ) } );
    if( !cubemap.IsValid() )
      return;


    const ModelAssetManager::Params meshParams
    {
      .mPath        { "assets/editor/Box.gltf"},
      .mOptVtxDecls { mVertexDecls},
    };

    TAC_CALL( Mesh * boxMesh{ ModelAssetManager::GetMesh( meshParams, errors ) } );
    if( !boxMesh )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::IDevice::Info renderDeviceInfo{ renderDevice->GetInfo() };
    const Render::NDCAttribs ndcAttribs{ renderDeviceInfo.mNDCAttribs };
    const float aspect { ( float )viewSize.x / ( float )viewSize.y };
    const m4::ProjectionMatrixParams projMtxParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspect },
      .mFOVYRadians   { camera->mFovyrad },
    };

    const v3 zeroCameraPos{ 0, 0, 0 };

    const m4 view{ m4::ViewInv( zeroCameraPos,
                                camera->mForwards,
                                camera->mRight,
                                camera->mUp ) };
    const m4 proj{ m4::ProjPerspective( projMtxParams ) };

    const SkyboxConstantBuffer skyboxConstantBuffer
    {
      .mView{ view },
      .mProj{ proj },
    };

    TAC_ASSERT( !boxMesh->mSubMeshes.empty() );
    const SubMesh* subMesh{ &boxMesh->mSubMeshes[ 0 ] };

    const Render::UpdateBufferParams updatePerFrame
    {
      .mSrcBytes     { &skyboxConstantBuffer },
      .mSrcByteCount { sizeof( SkyboxConstantBuffer ) },
    };

    const Render::DrawArgs drawArgs
    {
      .mIndexCount { subMesh->mIndexCount },
      .mStartIndex {},
    };

    sShaderCubemap->SetTexture( cubemap );
    sShaderConstantBuffer->SetBuffer( sConstantBuffer );
    sShaderSampler->SetSampler( mSamplerState );

    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    TAC_CALL( renderContext->UpdateBuffer( Render::DefaultCBufferPerFrame::sHandle,
                                           updatePerFrame,
                                           errors ) );
    renderContext->SetVertexBuffer( subMesh->mVertexBuffer );
    renderContext->SetIndexBuffer( subMesh->mIndexBuffer );
    renderContext->SetPipeline( sPipeline );
    //Render::SetVertexFormat( mVertexFormat );
    //Render::SetShader( mShader );
    //Render::SetBlendState( mBlendState );
    //Render::SetDepthState( mDepthState );
    //Render::SetRasterizerState( mRasterizerState );
    //Render::SetSamplerState( { mSamplerState } );
    //Render::Submit( viewId, TAC_STACK_FRAME );
    renderContext->CommitShaderVariables();
    renderContext->Draw( drawArgs );
  }

  void SkyboxPresentation::DebugImGui()
{
  ImGuiCheckbox( "Skybox Enabled", &mRenderEnabledSkybox );
}

} // namespace Tac

#endif
