#include "tac_render_tutorial_3_texture.h" // self-inc

#include "tac_render_tutorial.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct Vertex
  {
    NDCSpacePosition3 mPos;
    TextureCoordinate2 mUV;
  };

  static Vector< Vertex > GetVerts()
  {

    // Pos in NDC space, UV in DirectX (Y-) space, ccw winding
    // 0 3    0      0 3
    // 1 2    1 2      2
    const Vertex v0
    {
      .mPos { NDCSpacePosition3{ -1, 1, 0 } },
      .mUV  { TextureCoordinate2{ 0, 0 } },
    };

    const Vertex v1
    {
      .mPos { NDCSpacePosition3{ -1, -1, 0 } },
      .mUV  { TextureCoordinate2{ 0, 1 } },
    };

    const Vertex v2
    {
      .mPos { NDCSpacePosition3{ 1, -1, 0 } },
      .mUV  { TextureCoordinate2{ 1, 1 } },
    };

    const Vertex v3
    {
      .mPos { NDCSpacePosition3{ 1, 1, 0 } },
      .mUV  { TextureCoordinate2{ 1, 0 } },
    };

    const Monitor monitor { OS::OSGetPrimaryMonitor() };
    const float aspect{ ( float )monitor.mSize.x / ( float )monitor.mSize.y };
    Vector verts { v0, v1, v2, v0, v2, v3 };
    for( Vertex& vert : verts )
    {
      vert.mPos.mValue /= 2;
      vert.mPos.mValue.x /= aspect;
    }
    return verts;
  }

  // -----------------------------------------------------------------------------------------------


  HelloTexture::HelloTexture( App::Config cfg ) : App{ cfg } {}

  void HelloTexture::Init( Errors& errors )
  {
    mColorFormat = AppWindowApi::GetSwapChainColorFormat();
    mDepthFormat = AppWindowApi::GetSwapChainDepthFormat();
    TAC_CALL( mWindowHandle = RenderTutorialCreateWindow( mConfig.mName, errors ) );

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Vector< Vertex > verts{ GetVerts() };
    mVtxCount = verts.size();
    const Render::CreateBufferParams vtxBufParams
    {
      .mByteCount     { verts.size() * ( int )sizeof( Vertex ) },
      .mBytes         { verts.data() },
      .mStride        { sizeof( Vertex ) },
      .mUsage         { Render::Usage::Static },
      .mBinding       { Render::Binding::ShaderResource },
      .mGpuBufferMode { Render::GpuBufferMode::kByteAddress },
      .mOptionalName  { "tri verts" },
    };
    mVtxBuf = renderDevice->CreateBuffer( vtxBufParams, errors );

    const Render::ProgramParams programParams{ .mInputs { "HelloTexture" }, };
    TAC_CALL( mShader = renderDevice->CreateProgram( programParams, errors ) );

    const Render::CreateSamplerParams samplerParams
    {
      .mFilter{mFilter},
    };
    mSampler = renderDevice->CreateSampler( samplerParams );

    const Render::PipelineParams pipelineParams
    {
      .mProgram      { mShader },
      .mRTVColorFmts { mColorFormat },
      .mDSVDepthFmt  { mDepthFormat },
    };
    mPipeline = renderDevice->CreatePipeline( pipelineParams, errors );

    mShaderVtxBufs = renderDevice->GetShaderVariable( mPipeline, "BufferTable" );
    mShaderSamplers = renderDevice->GetShaderVariable( mPipeline, "Samplers" );
    mShaderTextures = renderDevice->GetShaderVariable( mPipeline, "Textures" );

    mShaderVtxBufs->SetResourceAtIndex( mVtxBuf, 0 );
    mShaderSamplers->SetResourceAtIndex( mSampler, 0 );

    // kickoff async loading
    TAC_CALL( TextureAssetManager::GetTexture( mTexPath, errors ) );
  }


  void HelloTexture::Render( RenderParams , Errors& errors )
  {
    
    if( !AppWindowApi::IsShown( mWindowHandle ) )
      return;

    if( !mTexture.IsValid() )
    {
      TAC_CALL( mTexture = TextureAssetManager::GetTexture( mTexPath, errors ) );
      if( !mTexture.IsValid() )
        return;

      mShaderTextures->SetResourceAtIndex( mTexture, 0 );
    }


    const v2i windowSize{ AppWindowApi::GetSize( mWindowHandle ) };

    Render::SwapChainHandle swapChain { AppWindowApi::GetSwapChainHandle( mWindowHandle ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( swapChain ) };
    Render::TextureHandle swapChainDepth { renderDevice->GetSwapChainDepth( swapChain ) };

    const Render::Targets renderTargets { .mColors{ swapChainColor } };
    const v4 clearColor{ 0.5f, 0.8f, 1, 0 };
    const Render::DrawArgs drawArgs { .mVertexCount { mVtxCount }, };

    TAC_CALL( Render::IContext::Scope renderContext{
      renderDevice->CreateRenderContext( errors ) } );
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetPipeline( mPipeline );
    renderContext->SetViewport( windowSize );
    renderContext->SetScissor( windowSize );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->ClearColor( swapChainColor, clearColor );
    renderContext->ClearDepth( swapChainDepth, 1 );
    renderContext->CommitShaderVariables();
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

