#include "tac_depth_buffer_visualizer.h" // self-inc

#include "tac-rhi/renderer/tac_renderer.h"

namespace Tac
{
  static Render::ShaderHandle         shader;
  static Render::SamplerStateHandle   samplerStateHandle;
  static Render::ConstantBufferHandle constantBuffer;
  static bool                         sInitialized;

  struct CBufferDepthViz
  {
    float projA;
    float projB;
    float f;
    float n;
  };

  static void DepthBufferVisuzliationInit()
  {
    if( sInitialized )
      return;

    sInitialized = true;

    samplerStateHandle = Render::CreateSamplerState( { .mFilter = Render::Filter::Point }, TAC_STACK_FRAME );

    constantBuffer = Render::CreateConstantBuffer( "CBufferVizf",
                                                   sizeof( CBufferDepthViz ),
                                                   TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( constantBuffer, "depth-buf-viz" );

    shader = Render::CreateShader(  "DepthBufferVisualizer" , TAC_STACK_FRAME );
  }
}

  Tac::Render::TextureHandle Tac::DepthBufferLinearVisualizationRender( const Render::TextureHandle& depthTexture,
                                                              int w,
                                                              int h,
                                                              const Render::InProj& inProj )
  {
    TAC_RENDER_GROUP_BLOCK( __func__ );

    const Render::OutProj outProj = Render::GetPerspectiveProjectionAB(inProj);

    const CBufferDepthViz cbuf =
    {
      .projA = outProj.mA,
      .projB = outProj.mB,
      .f = inProj.mFar,
      .n = inProj.mNear,
    };

    DepthBufferVisuzliationInit();

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
    const Render::TexSpec texSpec
    {
      .mImage = image,
      .mBinding = Render::Binding::RenderTarget | Render::Binding::ShaderResource,
    };

    Render::TextureHandle outTex = Render::CreateTexture( texSpec, TAC_STACK_FRAME );
    Render::ViewHandle viewHandle = Render::CreateView();
    Render::FramebufferHandle framebufferHandle =
      Render::CreateFramebufferForRenderToTexture( { outTex }, TAC_STACK_FRAME );
    Render::SetShader( shader );
    Render::SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, Render::Viewport( w, h ) );
    Render::SetViewScissorRect( viewHandle, Render::ScissorRect( w, h ) );
    Render::SetTexture( { depthTexture } );
    Render::UpdateConstantBuffer( constantBuffer, &cbuf, sizeof( CBufferDepthViz ), TAC_STACK_FRAME );
    Render::SetVertexBuffer( Render::VertexBufferHandle(), 0, 3 );
    Render::SetVertexFormat( Render::VertexFormatHandle() );
    Render::SetSamplerState( { samplerStateHandle } );
    Render::Submit( viewHandle, TAC_STACK_FRAME );
    Render::DestroyView( viewHandle );
    Render::DestroyFramebuffer( framebufferHandle, TAC_STACK_FRAME );
    return outTex;
  }
