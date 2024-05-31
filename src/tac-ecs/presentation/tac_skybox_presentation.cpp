#include "tac_skybox_presentation.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/profile/tac_profile.h"

#if TAC_SKYBOX_PRESENTATION_ENABLED()

namespace Tac
{
  //static Render::VertexFormatHandle    mVertexFormat;
  static Render::ProgramHandle          mShader;
  //static Render::BlendStateHandle      mBlendState;
  //static Render::DepthStateHandle      mDepthState;
  //static Render::RasterizerStateHandle mRasterizerState;
  static Render::SamplerHandle         mSamplerState;
  static Render::VertexDeclarations    mVertexDecls;
  static Render::PipelineHandle        sPipeline;
  static Render::BufferHandle          sConstantBuffer;
  static Render::IShaderVar*           sShaderConstantBuffer;
  static Render::IShaderVar*           sShaderCubemap;
  static Render::IShaderVar*           sShaderSampler;

  struct SkyboxConstantBuffer
  {
    m4 mView;
    m4 mProj;
  };
}

void Tac::SkyboxPresentationUninit()
{
  Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
  renderDevice->DestroyProgram( mShader );
  //Render::DestroyVertexFormat( mVertexFormat, TAC_STACK_FRAME );
}

void Tac::SkyboxPresentationInit( Errors& errors )
{

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
    .mMultisample           { false },
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
}

void Tac::SkyboxPresentationRender( SkyboxRenderParams params, Errors& errors )
{
  const Camera* camera{ params.mCamera };
  const int viewWidth{ params.mViewSize.x };
  const int viewHeight{ params.mViewSize.y };
  const Render::TextureHandle viewId{ params.mViewId };
  const AssetPathStringView skyboxDir{ params.mSkyboxDir };

  /*TAC_PROFILE_BLOCK*/;
  const AssetPathStringView defaultSkybox{ "assets/skybox/daylight" };
  const AssetPathStringView skyboxDirToUse{ skyboxDir.empty() ? defaultSkybox : skyboxDir };
  TAC_CALL( const Render::TextureHandle cubemap{ TextureAssetManager::GetTextureCube( skyboxDirToUse, errors ) } );
  if( !cubemap.IsValid() )
    return;

  TAC_CALL( Mesh * boxMesh{ ModelAssetManagerGetMeshTryingNewThing( "assets/editor/Box.gltf",
                                                       0,
                                                       mVertexDecls,
                                                       errors ) } );
  if( !boxMesh )
    return;

  Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
  const Render::IDevice::Info renderDeviceInfo{ renderDevice->GetInfo() };
  const Render::NDCAttribs ndcAttribs{ renderDeviceInfo.mNDCAttribs };
  const float aspect { ( float )viewWidth / ( float )viewHeight };
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

  TAC_CALL( Render::IContext::Scope renderContext{ renderDevice->CreateRenderContext( errors ) } );

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

  renderContext->SetViewport( params.mViewSize );
  renderContext->SetScissor( params.mViewSize );
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
  TAC_CALL( renderContext->Execute( errors ) );
}

#endif
