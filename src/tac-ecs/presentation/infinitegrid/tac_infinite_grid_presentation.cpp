#include "tac_infinite_grid_presentation.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"

#if TAC_INFINITE_GRID_PRESENTATION_ENABLED()

namespace Tac
{

  struct ConstantBuffer
  {
    m4 mInvView;
    m4 mInvProj;
    m4 mViewProj;
    v4 mCamPos_ws;
  };

  static Render::ProgramHandle         sProgram;
  static Render::PipelineHandle        sPipeline;
  static Render::IShaderVar*           sShaderConstants;
  static Render::BufferHandle          sConstantBuffer;
  static Render::UpdateBufferParams    sConstantUpdateParams;
  static ConstantBuffer                sConstantData;
  static bool                          sInitialized;
  static bool                          sEnabled { true };

  // -----------------------------------------------------------------------------------------------

  static void UpdateConstantData( const Camera* camera, const v2i viewSize )
  {
    const float aspectRatio{ ( float )viewSize.x / ( float )viewSize.y };
    const m4 invView{ camera->ViewInv() };
    const m4 invProj{ camera->ProjInv( aspectRatio ) };
    const m4 proj{ camera->Proj( aspectRatio ) };
    const m4 view{ camera->View() };
    const m4 viewProj{ proj * view };

    const ConstantBuffer constantBuffer
    {
      .mInvView   { invView },
      .mInvProj   { invProj },
      .mViewProj  { viewProj },
      .mCamPos_ws { camera->mPos, 1.0f },
    };
    sConstantData = constantBuffer;
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



  // -----------------------------------------------------------------------------------------------

  void InfiniteGrid::Init( Errors& errors )
  {
    if( sInitialized )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mInputs   { "InfiniteGrid" },
    };
    sProgram = renderDevice->CreateProgram( programParams, errors );


    const Render::BlendState blendState{ GetBlendState() };
    const Render::DepthState depthState{ GetDepthState() };
    const Render::RasterizerState rasterizerState{ GetRasterizerState() };

    const Render::PipelineParams pipelineParams
    {
      .mProgram           { sProgram },
      .mBlendState        { blendState},
      .mDepthState        { depthState},
      .mRasterizerState   { rasterizerState },
      .mRTVColorFmts      { Render::TexFmt::kRGBA16F },
      .mDSVDepthFmt       { Render::TexFmt::kD24S8 },
      .mPrimitiveTopology { Render::PrimitiveTopology::TriangleList },
      .mName              { "infinite-grid-pso" },
    };

    TAC_CALL( sPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    const Render::CreateBufferParams constBufParams
    {
      .mByteCount     { sizeof( ConstantBuffer ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mOptionalName  { "infinite-grid-cbuf" },
    };

    TAC_CALL( sConstantBuffer = renderDevice->CreateBuffer( constBufParams, errors ) );

    sShaderConstants = renderDevice->GetShaderVariable( sPipeline, "sConstants" );
    sShaderConstants->SetResource( sConstantBuffer );

    sConstantUpdateParams = Render::UpdateBufferParams 
    {
      .mSrcBytes     { &sConstantData },
      .mSrcByteCount { sizeof( ConstantBuffer ) },
    };

    sInitialized = true;
  }

  void InfiniteGrid::Uninit()
  {
    if( sInitialized )
    {
      Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
      renderDevice->DestroyProgram( sProgram );
      renderDevice->DestroyPipeline( sPipeline );
      sInitialized = false;
    }

  }


  void InfiniteGrid::Render( Render::IContext* renderContext,
                             const Camera* camera,
                             const v2i viewSize,
                             const Render::TextureHandle dstColorTex,
                             const Render::TextureHandle dstDepthTex,
                             Errors& errors )
  {
    if( !sEnabled )
      return;

    const Render::Targets renderTargets
    {
      .mColors { dstColorTex },
      .mDepth  { dstDepthTex },
    };

    const Render::DrawArgs drawArgs
    {
      .mVertexCount { 6 },
    };

    UpdateConstantData( camera, viewSize );

    TAC_RENDER_GROUP_BLOCK( renderContext, "Infinite Grid" );
    renderContext->SetViewport( viewSize );
    renderContext->SetScissor( viewSize );
    renderContext->SetRenderTargets( renderTargets );
    TAC_CALL( renderContext->UpdateBuffer( sConstantBuffer, sConstantUpdateParams, errors ) );
    renderContext->SetPipeline( sPipeline );
    renderContext->CommitShaderVariables();
    renderContext->SetVertexBuffer( {} );
    renderContext->SetIndexBuffer( {} );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->Draw( drawArgs );
  }

  void InfiniteGrid::DebugImGui()
  {
    ImGuiCheckbox( "Infinite Grid", &sEnabled );
  }

} // namespace Tac
#endif
