#include "tac_level_editor_icon_renderer.h" // self-inc
#include "tac_level_editor_light_widget.h"

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"

#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-std-lib/math/tac_matrix4.h"

#if TAC_IS_ICON_RENDERER_ENABLED()

namespace Tac
{
  struct PerFrame
  {
    m4 mView;
    m4 mProj;
  };

  struct PerObj
  {
    m4 mWorld;
  };

  static Render::ProgramHandle         sSpriteShader             {};
  static Render::PipelineHandle        sSpritePipeline           {};
  static Render::BufferHandle          sPerFrame                 {};
  static Render::BufferHandle          sPerObj                   {};
  static Render::SamplerHandle         sSampler                  {};
  static Render::IShaderVar*           sShaderPerFrame           {};
  static Render::IShaderVar*           sShaderPerObj             {};
  static Render::IShaderVar*           sShaderTexture            {};
  static Render::IShaderVar*           sShaderSampler            {};

  static auto GetProj( WindowHandle viewHandle, const Camera* camera ) -> m4
  {
    
    const v2i windowSize{ AppWindowApi::GetSize( viewHandle ) };
    const float aspectRatio{ ( float )windowSize.x / ( float )windowSize.y };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs { renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspectRatio },
      .mFOVYRadians   { camera->mFovyrad },
    };
    return m4::ProjPerspective( projParams );
  }
  static void IconRendererUpdatePerFrame( Render::IContext* renderContext,
                                     const WindowHandle viewHandle,
                                     const Camera* camera,
                                     Errors& errors )
  {
    const m4 view { camera->View() };
    const m4 proj { GetProj( viewHandle, camera ) };
    const PerFrame perFrame
    {
      .mView{ view },
      .mProj{ proj },
    };

    const Render::UpdateBufferParams update
    {
      .mSrcBytes{ &perFrame },
      .mSrcByteCount{ sizeof( PerFrame ) },
    };

    renderContext->UpdateBuffer( sPerFrame, update, errors );
  }

  static void IconRendererUpdatePerObj( Render::IContext* renderContext,
                                   const Light* light,
                                   Errors& errors )
  {

    // note: Only scaling the m00, quad x and y scale are handled by m00, see 3DSprite.hlsl
    m4 world { light->mEntity->mWorldTransform };
    world.m00 = LightWidget::sSize;

    const PerObj perObj
    {
      .mWorld { world },
    };

    const Render::UpdateBufferParams update
    {
      .mSrcBytes{ &perObj },
      .mSrcByteCount{ sizeof( PerObj ) },
    };

    renderContext->UpdateBuffer( sPerObj, update, errors );
  }


  void IconRenderer::Init( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    sSampler = renderDevice->CreateSampler(
      Render::CreateSamplerParams
      {
        .mFilter{ Render::Filter::Linear },
        .mName  { "icon_lin_sampler" },
      } );
    TAC_CALL( sSpriteShader = renderDevice->CreateProgram(
      Render::ProgramParams
      {
        .mInputs     { "3DSprite" },
        .mStackFrame { TAC_STACK_FRAME },
      }, errors ) );
    TAC_CALL( sPerFrame = renderDevice->CreateBuffer(
      Render::CreateBufferParams
      {
        .mByteCount     { sizeof( PerFrame ) },
        .mUsage         { Render::Usage::Dynamic },
        .mBinding       { Render::Binding::ConstantBuffer },
        .mOptionalName  { "icon per frame" },
      }, errors ) );
    TAC_CALL( sPerObj = renderDevice->CreateBuffer(
      Render::CreateBufferParams
      {
        .mByteCount     { sizeof( PerObj ) },
        .mUsage         { Render::Usage::Dynamic },
        .mBinding       { Render::Binding::ConstantBuffer },
        .mOptionalName  { "icon per obj" },
      }, errors ) );
    TAC_CALL( sSpritePipeline = renderDevice->CreatePipeline(
      Render::PipelineParams
      {
        .mProgram           { sSpriteShader },
        .mBlendState        { Render::BlendState
            {
              .mSrcRGB   { Render::BlendConstants::SrcA },
              .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
              .mBlendRGB { Render::BlendMode::Add },
              .mSrcA     { Render::BlendConstants::Zero },
              .mDstA     { Render::BlendConstants::One },
              .mBlendA   { Render::BlendMode::Add},
            } },
        .mDepthState        { Render::DepthState
          {
            .mDepthTest  { true },
            .mDepthWrite { true },
            .mDepthFunc  { Render::DepthFunc::Less },
          } },
        .mRasterizerState   { Render::RasterizerState
          {
            .mFillMode              { Render::FillMode::Solid },
            .mCullMode              { Render::CullMode::None },
            .mFrontCounterClockwise { true },
            .mMultisample           {},
          } },
        .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
        .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
        .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
        .mName              { "IconPipeline"},
      }, errors ) );
    sShaderPerFrame = renderDevice->GetShaderVariable( sSpritePipeline, "perFrame" );
    sShaderPerObj   = renderDevice->GetShaderVariable( sSpritePipeline, "perObj" );
    sShaderTexture  = renderDevice->GetShaderVariable( sSpritePipeline, "sprite" );
    sShaderSampler  = renderDevice->GetShaderVariable( sSpritePipeline, "linearSampler" );
    sShaderPerFrame->SetResource( sPerFrame );
    sShaderPerObj->SetResource( sPerObj );
    sShaderSampler->SetResource( sSampler );
  }

  void IconRenderer::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( sSpriteShader);
    renderDevice->DestroyPipeline( sSpritePipeline);
  }



  void IconRenderer::RenderLights( const World* world,
                                   const Camera* camera,
                                   Render::IContext* renderContext,
                                   WindowHandle viewHandle,
                                   Errors& errors )
  {
    TAC_RENDER_GROUP_BLOCK( renderContext, "light widgets" );

    const Render::TextureHandle textureHandle{
      TextureAssetManager::GetTexture( "assets/editor/light.png", errors ) };

    if( !textureHandle.IsValid() )
      return;

    struct : public LightVisitor
    {
      void operator()( Light* light ) override { mLights.push_back( light ); }
      Vector< const Light* > mLights;
    } lightVisitor;

    
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Graphics* graphics { Graphics::From( world ) };
    const v2i windowSize{ AppWindowApi::GetSize( viewHandle ) };
    Render::SwapChainHandle swapChain { AppWindowApi::GetSwapChainHandle( viewHandle ) };
    Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( swapChain ) };
    Render::TextureHandle swapChainDepth { renderDevice->GetSwapChainDepth( swapChain ) };
    const Render::Targets renderTargets
    {
      .mColors{ swapChainColor },
      .mDepth{ swapChainDepth },
    };

    const Render::DrawArgs drawArgs
    {
      .mVertexCount { 6 },
    };

    sShaderTexture->SetResource( textureHandle );
    renderContext->SetScissor( windowSize );
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( windowSize );
    renderContext->SetPipeline( sSpritePipeline );
    renderContext->SetVertexBuffer( {} );
    renderContext->SetIndexBuffer( {} );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    TAC_CALL( IconRendererUpdatePerFrame( renderContext, viewHandle, camera, errors ) );
    graphics->VisitLights( &lightVisitor );

    for( int iLight {}; iLight < lightVisitor.mLights.size(); ++iLight )
    {
      const Light* light { lightVisitor.mLights[ iLight ] };
      const String groupname{ String() + "Editor light " + ToString( iLight ) };

      TAC_RENDER_GROUP_BLOCK( renderContext, groupname );
      TAC_CALL( IconRendererUpdatePerObj( renderContext, light, errors ) );

      renderContext->CommitShaderVariables();
      renderContext->Draw( drawArgs );
    }
  }
} // namespace Tac
#endif
