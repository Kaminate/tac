#include "tac_level_editor_icon_renderer.h" // self-inc
#include "tac_level_editor_light_widget.h"

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-std-lib/math/tac_matrix4.h"


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

  // -----------------------------------------------------------------------------------------------

  static Render::ProgramParams GetProgramParams3DSprite()
  {
    return Render::ProgramParams
    {
      .mFileStem   { "3DSprite" },
      .mStackFrame { TAC_STACK_FRAME },
    };
  }

  static Render::CreateSamplerParams GetSamplerParams()
  {
    return Render::CreateSamplerParams
    {
      .mFilter{ Render::Filter::Linear },
    };
  }

  static Render::CreateBufferParams GetPerObjParams()
  {
    return Render::CreateBufferParams
    {
      .mByteCount     { sizeof( PerObj ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "icon per obj" },
    };
  }

  static Render::CreateBufferParams GetPerFrameParams()
  {
    return Render::CreateBufferParams
    {
      .mByteCount     { sizeof( PerFrame ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "icon per frame" },
    };
  }

  static Render::BlendState GetBlendState()
  {
    return Render::BlendState
    {
      .mSrcRGB   { Render::BlendConstants::SrcA },
      .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add},
    };
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

  static m4 GetProj( WindowHandle viewHandle, const Camera* camera )
  {
    SysWindowApi windowApi;
    const v2i windowSize{ windowApi.GetSize( viewHandle ) };
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

  static Render::RasterizerState GetRasterizerState()
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mMultisample           { false },
    };
  }

  // -----------------------------------------------------------------------------------------------
  Render::PipelineParams IconRenderer::GetPipelineParams()
  {
    return Render::PipelineParams
    {
      .mProgram           { mSpriteShader },
      .mBlendState        { GetBlendState() },
      .mDepthState        { GetDepthState() },
      .mRasterizerState   { GetRasterizerState() },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "IconPipeline"},
    };
  }


  void IconRenderer::Init( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    mSampler = renderDevice->CreateSampler( GetSamplerParams() );
    TAC_CALL( mSpriteShader = renderDevice->CreateProgram( GetProgramParams3DSprite(), errors ) );
    TAC_CALL( mPerFrame = renderDevice->CreateBuffer( GetPerFrameParams(), errors ) );
    TAC_CALL( mPerObj = renderDevice->CreateBuffer( GetPerObjParams(), errors ) );
    TAC_CALL( mSpritePipeline = renderDevice->CreatePipeline( GetPipelineParams(), errors ) );
    mShaderPerFrame = renderDevice->GetShaderVariable( mSpritePipeline, "perFrame" );
    mShaderPerObj   = renderDevice->GetShaderVariable( mSpritePipeline, "perObj" );
    mShaderTexture  = renderDevice->GetShaderVariable( mSpritePipeline, "sprite" );
    mShaderSampler  = renderDevice->GetShaderVariable( mSpritePipeline, "linearSampler" );

    mShaderPerFrame->SetBuffer( mPerFrame );
    mShaderPerObj->SetBuffer( mPerObj );
    mShaderSampler->SetSampler( mSampler );
  }

  void IconRenderer::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( mSpriteShader);
    renderDevice->DestroyPipeline( mSpritePipeline);
  }

  void IconRenderer::UpdatePerFrame( Render::IContext* renderContext,
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

    renderContext->UpdateBuffer( mPerFrame, update, errors );
  }

  void IconRenderer::UpdatePerObj( Render::IContext* renderContext,
                                   const Light* light ,
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

    renderContext->UpdateBuffer( mPerObj, update, errors );
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

    SysWindowApi windowApi;
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Graphics* graphics { GetGraphics( world ) };
    const v2i windowSize{ windowApi.GetSize( viewHandle ) };
    Render::SwapChainHandle swapChain { windowApi.GetSwapChainHandle( viewHandle ) };
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

    mShaderTexture->SetTexture( textureHandle );
    renderContext->SetScissor( windowSize );
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetViewport( windowSize );
    renderContext->SetPipeline( mSpritePipeline );
    renderContext->SetVertexBuffer( {} );
    renderContext->SetIndexBuffer( {} );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    TAC_CALL( UpdatePerFrame( renderContext, viewHandle, camera, errors ) );
    graphics->VisitLights( &lightVisitor );

    for( int iLight {}; iLight < lightVisitor.mLights.size(); ++iLight )
    {
      const Light* light { lightVisitor.mLights[ iLight ] };
      const String groupname{ String() + "Editor light " + ToString( iLight ) };

      TAC_RENDER_GROUP_BLOCK( renderContext, groupname );
      TAC_CALL( UpdatePerObj( renderContext, light, errors ) );

      renderContext->CommitShaderVariables();
      renderContext->Draw( drawArgs );
    }
  }
} // namespace Tac
