#include "tac_depth_buffer_visualizer.h" // self-inc

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"


namespace Tac
{
  static Render::ProgramHandle        sShaderProgram;
#if TAC_TEMPORARILY_DISABLED()
  static Render::SamplerStateHandle   sSamplerState;
#endif
  static Render::BufferHandle         sConstantBuffer;
  static bool                         sInitialized;

  struct CBufferDepthViz
  {
    float projA;
    float projB;
    float f;
    float n;
  };

  static void DepthBufferVisuzliationInit( Errors& errors )
  {
    if( sInitialized )
      return;

    sInitialized = true;

#if TAC_TEMPORARILY_DISABLED()
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    sSamplerState = Render::CreateSamplerState( { .mFilter = Render::Filter::Point }, TAC_STACK_FRAME );

    Render::CreateBufferParams createBufferParams
    {
      .mByteCount = sizeof( CBufferDepthViz ),
      .mAccess = Render::Usage::Dynamic,
      .mOptionalName =  "CBufferVizf",
      .mStackFrame = TAC_STACK_FRAME,
    };
    sConstantBuffer = TAC_CALL( renderDevice->CreateBuffer( createBufferParams, errors ) );

    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();

    Render::ProgramParams programParams
    {
      .mFileStem =  "DepthBufferVisualizer",
    };
    sShaderProgram = renderDevice->CreateProgram( programParams, errors );
#endif
  }
}

Tac::Render::TextureHandle Tac::DepthBufferLinearVisualizationRender(
  const Render::TextureHandle& depthTexture,
  int w,
  int h,
  Errors& errors )
{

#if TAC_TEMPORARILY_DISABLED()
  renderContext->DebugEventBegin( __func__ );

  const Render::OutProj outProj = Render::GetPerspectiveProjectionAB(inProj);

  const CBufferDepthViz cbuf =
  {
    .projA = outProj.mA,
    .projB = outProj.mB,
    .f = inProj.mFar,
    .n = inProj.mNear,
  };

  DepthBufferVisuzliationInit( errors );

  const Render::Format format =
  {
        .mElementCount = 4,
        .mPerElementByteCount = 1,
        .mPerElementDataType = Render::GraphicsType::unorm
  };
  const Render::Image image = 
  {
      .mWidth = w,
      .mHeight = h,
      .mFormat = format,
  };
  const Render::CreateTextureParams CreateTextureParams
  {
    .mImage = image,
    .mBinding = Render::Binding::RenderTarget | Render::Binding::ShaderResource,
  };

  Render::TextureHandle outTex = Render::CreateTexture( CreateTextureParams, TAC_STACK_FRAME );
  Render::ViewHandle viewHandle = Render::CreateView();
  Render::FramebufferHandle framebufferHandle =
    Render::CreateFramebufferForRenderToTexture( { outTex }, TAC_STACK_FRAME );
  Render::SetShader( sShaderProgram );
  Render::SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
  Render::SetViewFramebuffer( viewHandle, framebufferHandle );
  Render::SetViewport( viewHandle, Render::Viewport( w, h ) );
  Render::SetViewScissorRect( viewHandle, Render::ScissorRect( w, h ) );
  Render::SetTexture( { depthTexture } );
  Render::UpdateConstantBuffer( sConstantBuffer, &cbuf, sizeof( CBufferDepthViz ), TAC_STACK_FRAME );
  Render::SetVertexBuffer( Render::BufferHandle(), 0, 3 );
  Render::SetVertexFormat( Render::VertexFormatHandle() );
  Render::SetSamplerState( { sSamplerState } );
  Render::Submit( viewHandle, TAC_STACK_FRAME );
  Render::DestroyView( viewHandle );
  Render::DestroyFramebuffer( framebufferHandle, TAC_STACK_FRAME );
  renderContext->DebugEventEnd( __func__ );
  return outTex;
#else
  return {};
#endif
}
