#include "tac_render_tutorial.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct Vertex
  {
    ClipSpacePosition3 mPos;
    LinearColor3       mCol;
  };


  // -----------------------------------------------------------------------------------------------

  struct HelloTexture : public App
  {
    HelloTexture( App::Config );

  protected:
    void Init( InitParams, Errors& ) override;
    void Render( RenderParams, Errors& ) override;

  private:
    void InitVertexBuffer( Errors& );
    void InitShader( Errors& );
    void InitRootSig( Errors& );

    WindowHandle           mWindowHandle;
    Render::BufferHandle   mVtxBuf;
    Render::ProgramHandle  mShader;
    Render::PipelineHandle mPipeline;
    Render::TexFmt         mColorFormat;
    Render::TexFmt         mDepthFormat;
  };

  HelloTexture::HelloTexture( App::Config cfg ) : App{ cfg } {}

  void HelloTexture::Init( InitParams initParams, Errors& errors )
  {
    const SysWindowApi* windowApi{ initParams.mWindowApi };
    mColorFormat = windowApi->GetSwapChainColorFormat();
    mDepthFormat = windowApi->GetSwapChainDepthFormat();
    TAC_CALL( mWindowHandle = RenderTutorialCreateWindow(
      initParams.mWindowApi, "Hello Texture", errors ) );
    InitVertexBuffer( errors );
    InitShader( errors );
    InitRootSig( errors );
  }

  void HelloTexture::InitShader( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams{ .mFileStem { "HelloTexture" }, };
    TAC_CALL( mShader = renderDevice->CreateProgram( programParams, errors ) );
  }

  void HelloTexture::InitVertexBuffer( Errors& errors )
  {
    const Vertex triangleVertices[]
    {
      Vertex
      {
        .mPos { ClipSpacePosition3{0.0f, 0.25f , 0.0f} },
        .mCol { LinearColor3{ 1.0f, 0.0f, 0.0f }}
      },
      Vertex
      {
        .mPos { ClipSpacePosition3{ -0.25f, -0.25f , 0.0f} },
        .mCol { LinearColor3{ 0.0f, 1.0f, 0.0f }}
      },
      Vertex
      {
        .mPos { ClipSpacePosition3{ 0.25f, -0.25f , 0.0f} },
        .mCol { LinearColor3{ 0.0f, 0.0f, 1.0f }}
      },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateBufferParams params
    {
      .mByteCount     { sizeof( triangleVertices ) },
      .mBytes         { triangleVertices },
      .mStride        { sizeof( Vertex ) },
      .mUsage         { Render::Usage::Static },
      .mBinding       { Render::Binding::ShaderResource | Render::Binding::VertexBuffer },
      .mGpuBufferMode { Render::GpuBufferMode::kByteAddress },
      .mOptionalName  { "tri verts" },
    };
    mVtxBuf = renderDevice->CreateBuffer( params, errors );
  }

  void HelloTexture::InitRootSig( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::PipelineParams pipelineParams
    {
      .mProgram      { mShader },
      .mRTVColorFmts { mColorFormat },
      .mDSVDepthFmt  { mDepthFormat },
    };

    mPipeline = renderDevice->CreatePipeline( pipelineParams, errors );
  }

  void HelloTexture::Render( RenderParams sysRenderParams, Errors& errors )
  {
    const SysWindowApi* windowApi{ sysRenderParams.mWindowApi };
    const v2i windowSize{ windowApi->GetSize( mWindowHandle ) };
    Render::SwapChainHandle swapChain { windowApi->GetSwapChainHandle( mWindowHandle ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( swapChain ) };
    Render::TextureHandle swapChainDepth { renderDevice->GetSwapChainDepth( swapChain ) };

    TAC_CALL( Render::IContext::Scope renderContext{ renderDevice->CreateRenderContext( errors ) } );
    const Render::Targets renderTargets { .mColors{ swapChainColor } };
    const v4 clearColor{ 0.5f, 0.8f, 1, 0 };

    renderContext->SetRenderTargets( renderTargets );

    renderContext->SetPipeline( mPipeline );

    renderContext->SetViewport( windowSize );
    renderContext->SetScissor( windowSize );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->ClearColor( swapChainColor, clearColor );
    renderContext->ClearDepth( swapChainDepth, 1 );

    // TODO: store this variable
    Render::IShaderVar* shaderVar {
      renderDevice->GetShaderVariable( mPipeline, "BufferTable" ) };

    shaderVar->SetBufferAtIndex( 0, mVtxBuf );
    renderContext->CommitShaderVariables();

    const Render::DrawArgs drawArgs
    {
      .mVertexCount { 3 },
    };
    renderContext->Draw( drawArgs );

    TAC_CALL( renderContext->Execute( errors ) );
    
    TAC_CALL( renderDevice->Present( swapChain, errors ) );
  }

  App* App::Create()
  {
    const App::Config config
    {
      .mName { "Hello Texture" },
    };
    return TAC_NEW HelloTexture( config );
  };

} // namespace Tac

