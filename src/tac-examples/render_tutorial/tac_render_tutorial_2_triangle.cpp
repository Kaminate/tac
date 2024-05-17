#include "tac_render_tutorial.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------


  struct Vertex
  {
    NDCSpacePosition3 mPos;
    LinearColor3       mCol;
  };

  // -----------------------------------------------------------------------------------------------

  static Render::VertexDeclarations GetVertexDeclarations()
  {
    const Render::VertexDeclaration vtxDeclPos
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::Format::sv3 },
      .mAlignedByteOffset { TAC_OFFSET_OF( Vertex, mPos ) },
    };

    const Render::VertexDeclaration vtxDeclCol
    {
      .mAttribute         { Render::Attribute::Color },
      .mFormat            { Render::Format::sv3 },
      .mAlignedByteOffset { TAC_OFFSET_OF( Vertex, mCol ) },
    };

    Render::VertexDeclarations vtxDecls;
    vtxDecls.push_back( vtxDeclPos );
    vtxDecls.push_back( vtxDeclCol );
    return vtxDecls;
  }

  // -----------------------------------------------------------------------------------------------

  struct HelloTriangle : public App
  {
    HelloTriangle( App::Config );

  protected:
    void Init( InitParams, Errors& ) override;
    void Render( RenderParams, Errors& ) override;

  private:
    void InitWindow( InitParams, Errors& );
    void InitVertexBuffer( Errors& );
    void InitShader( Errors& );
    void InitRootSig( Errors& );

    WindowHandle           sWindowHandle;
    Render::BufferHandle   mVtxBuf;
    Render::ProgramHandle  mShaderBindless;
    Render::ProgramHandle  mShaderInputLayout;
    Render::PipelineHandle mPipelineBindless;
    Render::PipelineHandle mPipelineInputLayout;
    Render::TexFmt         mColorFormat;
    Render::TexFmt         mDepthFormat;
    bool                   mBindless{ true };
  };

  HelloTriangle::HelloTriangle( App::Config cfg ) : App{ cfg } {}

  void HelloTriangle::Init( InitParams initParams, Errors& errors )
  {
    const SysWindowApi* windowApi{ initParams.mWindowApi };
    mColorFormat = windowApi->GetSwapChainColorFormat();
    mDepthFormat = windowApi->GetSwapChainDepthFormat();

    InitWindow( initParams, errors );
    InitVertexBuffer( errors );
    InitShader( errors );
    InitRootSig( errors );
  }

  void HelloTriangle::InitWindow( InitParams initParams, Errors& errors )
  {
      TAC_CALL( sWindowHandle = RenderTutorialCreateWindow(
        initParams.mWindowApi, mConfig.mName, errors ) );
  }

  void HelloTriangle::InitShader( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams paramsBindless    { .mFileStem { "HelloTriangleBindless" }, };
    const Render::ProgramParams paramsInputLayout { .mFileStem { "HelloTriangleInputLayout" }, };
    TAC_CALL( mShaderInputLayout = renderDevice->CreateProgram( paramsInputLayout, errors ) );
    TAC_CALL( mShaderBindless = renderDevice->CreateProgram( paramsBindless, errors ) );
  }

  void HelloTriangle::InitVertexBuffer( Errors& errors )
  {
    const Vertex triangleVertices[]
    {
      Vertex
      {
        .mPos { NDCSpacePosition3{0.0f, 0.25f , 0.0f} },
        .mCol { LinearColor3{ 1.0f, 0.0f, 0.0f }}
      },
      Vertex
      {
        .mPos { NDCSpacePosition3{ -0.25f, -0.25f , 0.0f} },
        .mCol { LinearColor3{ 0.0f, 1.0f, 0.0f }}
      },
      Vertex
      {
        .mPos { NDCSpacePosition3{ 0.25f, -0.25f , 0.0f} },
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

  void HelloTriangle::InitRootSig( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::VertexDeclarations vtxDecls{ GetVertexDeclarations() };

    const Render::PipelineParams paramsBindless
    {
      .mProgram      { mShaderBindless },
      .mRTVColorFmts { mColorFormat },
      .mDSVDepthFmt  { mDepthFormat },
    };

    const Render::PipelineParams paramsInputLayout
    {
      .mProgram      { mShaderInputLayout },
      .mRTVColorFmts { mColorFormat },
      .mDSVDepthFmt  { mDepthFormat },
      .mVtxDecls     { vtxDecls },
    };

    mPipelineBindless = renderDevice->CreatePipeline( paramsBindless, errors );
    mPipelineInputLayout = renderDevice->CreatePipeline( paramsInputLayout, errors );
  }

  void HelloTriangle::Render( RenderParams sysRenderParams, Errors& errors )
  {
    // Test bindless vs not bindless by flipping it every frame
    mBindless = !mBindless;

    const SysWindowApi* windowApi{ sysRenderParams.mWindowApi };
    const v2i windowSize{ windowApi->GetSize( sWindowHandle ) };
    Render::SwapChainHandle swapChain { windowApi->GetSwapChainHandle( sWindowHandle ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( swapChain ) };
    Render::TextureHandle swapChainDepth { renderDevice->GetSwapChainDepth( swapChain ) };

    TAC_CALL( Render::IContext::Scope renderContext{ renderDevice->CreateRenderContext( errors ) } );
    const Render::Targets renderTargets { .mColors{ swapChainColor } };
    const v4 clearColor{ 0.5f, 0.8f, 1, 0 };

    renderContext->SetRenderTargets( renderTargets );

    if( mBindless )
      renderContext->SetPipeline( mPipelineBindless );
    else
      renderContext->SetPipeline( mPipelineInputLayout );

    renderContext->SetViewport( windowSize );
    renderContext->SetScissor( windowSize );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->ClearColor( swapChainColor, clearColor );
    renderContext->ClearDepth( swapChainDepth, 1 );

    if( mBindless )
    {
      Render::IShaderVar* shaderVar {
        renderDevice->GetShaderVariable( mPipelineBindless, "BufferTable" ) };
      shaderVar->SetBufferAtIndex( 0, mVtxBuf );
      renderContext->CommitShaderVariables();
    }
    else
    {
      renderContext->SetVertexBuffer( mVtxBuf );
    }

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
      .mName { "Hello Triangle" },
    };
    return TAC_NEW HelloTriangle( config );
  };

} // namespace Tac
