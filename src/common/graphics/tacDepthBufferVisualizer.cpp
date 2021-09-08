#include "src/common/graphics/tacDepthBufferVisualizer.h"

namespace Tac
{
#define TAC_CALL_ONCE { static bool initialized; if( initialized ) return; initialized = true; }

  static Render::ShaderHandle         shader;
  static Render::SamplerStateHandle   samplerStateHandle;
  static Render::ConstantBufferHandle constantBuffer;

  struct CBufferDepthViz
  {
    float projA;
    float projB;
    float f;
    float n;
  };

  static void DepthBufferVisuzliationInit()
  {
    TAC_CALL_ONCE;

    Render::SamplerState samplerState = {};
    samplerState.mFilter = Render::Filter::Point;
    samplerStateHandle = Render::CreateSamplerState( samplerState, TAC_STACK_FRAME );

    constantBuffer = Render::CreateConstantBuffer( "CBufferVizf",
                                                   sizeof( CBufferDepthViz ),
                                                   TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( constantBuffer, "depth-buf-viz" );

    shader = Render::CreateShader( Render::ShaderSource::FromPath( "DepthBufferVisualizer" ), TAC_STACK_FRAME );
  }

  Render::TextureHandle DepthBufferLinearVisualizationRender( Render::TextureHandle depthTexture,
                                                              int w,
                                                              int h,
                                                              float f,
                                                              float n )
  {
    TAC_RENDER_GROUP_BLOCK( __func__ );

    float a, b;
    Render::GetPerspectiveProjectionAB( f, n, a, b );

    CBufferDepthViz cbuf = {};
    cbuf.projA = a;
    cbuf.projB = b;
    cbuf.f = f;
    cbuf.n = n;

    DepthBufferVisuzliationInit();
    Render::TexSpec texSpec = {};
    texSpec.mBinding = Render::Binding::RenderTarget | Render::Binding::ShaderResource;
    texSpec.mImage.mWidth = w;
    texSpec.mImage.mHeight = h;
    texSpec.mImage.mFormat.mElementCount = 4;
    texSpec.mImage.mFormat.mPerElementByteCount = 1;
    texSpec.mImage.mFormat.mPerElementDataType = Render::GraphicsType::unorm;
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
    Render::SetSamplerState( samplerStateHandle );
    Render::Submit( viewHandle, TAC_STACK_FRAME );
    Render::DestroyView( viewHandle );
    Render::DestroyFramebuffer( framebufferHandle, TAC_STACK_FRAME );
    return outTex;
  }
}
