#include "tac_render_tutorial_4_const_buf.h" // self-inc

#include "tac_render_tutorial.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"

#define TAC_HELLO_BINDLESS() 1

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct MyCBufType
  {
    m4  mWorld;
    u32 mVertexBufferIndex;
  };

  struct Vertex
  {
    NDCSpacePosition3  mPos;
    LinearColor3       mCol;
  };

  static Vector< Vertex > GetVerts()
  {
    // Pos: NDC space, UV: DirectX (Y-) space, Winding: CCW
    //  0
    // 1 2
    const Vertex v0
    {
      .mPos { NDCSpacePosition3{ 0, 1, 0 } },
      .mCol { LinearColor3{ 1, 0, 0 } },
    };

    const Vertex v1
    {
      .mPos { NDCSpacePosition3{ -1, -1, 0 } },
      .mCol { LinearColor3{ 0, 1, 0 } },
    };

    const Vertex v2
    {
      .mPos { NDCSpacePosition3{ 1, -1, 0 } },
      .mCol { LinearColor3{ 0, 0, 1 } },
    };


    Vector verts { v0, v1, v2 };
    for( Vertex& vert : verts )
      vert.mPos.mValue /= 2;

    return verts;
  }

  // -----------------------------------------------------------------------------------------------

  HelloConstBuf::HelloConstBuf( App::Config cfg ) : App{ cfg } {}

  void HelloConstBuf::Init( Errors& errors )
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

    const Render::CreateBufferParams cBufParams
    {
      .mByteCount     { ( int )sizeof( MyCBufType ) },
      .mBytes         {},
      .mStride        {},
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ConstantBuffer },
      .mGpuBufferMode {},
      .mOptionalName  { "cbuf" },
    };
    mConstantBuf = renderDevice->CreateBuffer( cBufParams, errors );

    const Render::ProgramParams programParams{ .mInputs { "HelloConstBuf" }, };
    TAC_CALL( mShader = renderDevice->CreateProgram( programParams, errors ) );

    const Render::PipelineParams pipelineParams
    {
      .mProgram      { mShader },
      .mRTVColorFmts { mColorFormat },
      .mDSVDepthFmt  { mDepthFormat },
    };
    mPipeline = renderDevice->CreatePipeline( pipelineParams, errors );



    const Render::IBindlessArray::Params bindlessArrayParams
    {
      .mHandleType { Render::HandleType::kBuffer },
      .mBinding    { Render::Binding::ShaderResource },
    };

    mShaderVtxBufs = renderDevice->GetShaderVariable( mPipeline, "BufferTable" );
#if TAC_HELLO_BINDLESS()
    mBindlessArray = renderDevice->CreateBindlessArray( bindlessArrayParams );
    mShaderVtxBufs->SetBindlessArray( mBindlessArray );
    TAC_CALL( mBindlessVtxBufBinding = mBindlessArray->Bind( mVtxBuf, errors ) );
#else
    mShaderVtxBufs->SetResourceAtIndex( mVtxBuf, 0 );
#endif

    mShaderConstantBuffer = renderDevice->GetShaderVariable( mPipeline, "MyCBufInst" );
    mShaderConstantBuffer->SetResource( mConstantBuf );
  }

  void HelloConstBuf::Render( RenderParams renderParams, Errors& errors )
  {

    if( !AppWindowApi::IsShown( mWindowHandle ) )
      return;

    const v2i windowSize{ AppWindowApi::GetSize( mWindowHandle ) };

    Render::SwapChainHandle swapChain { AppWindowApi::GetSwapChainHandle( mWindowHandle ) };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::TextureHandle swapChainColor { renderDevice->GetSwapChainCurrentColor( swapChain ) };
    Render::TextureHandle swapChainDepth { renderDevice->GetSwapChainDepth( swapChain ) };

    const Render::Targets renderTargets { .mColors{ swapChainColor } };
    const v4 clearColor{ 0.5f, 0.8f, 1, 0 };
    const Render::DrawArgs drawArgs { .mVertexCount { mVtxCount }, };

#if TAC_HELLO_BINDLESS()
    TAC_ASSERT( mBindlessVtxBufBinding.IsValid() );
#endif

    const float translateX{ ( float )Sin( renderParams.mTimestamp.mSeconds / 2.0f ) / 2.0f };
    const m4 world{ m4::Translate( v3( translateX, 0, 0 ) ) };
    const MyCBufType cbuf
    {
      .mWorld             { world },

#if TAC_HELLO_BINDLESS()
      .mVertexBufferIndex { ( u32 )mBindlessVtxBufBinding.GetIndex() },
#else
      .mVertexBufferIndex {},
#endif
    };

    TAC_CALL( Render::IContext::Scope renderContext{
      renderDevice->CreateRenderContext( errors ) } );
    renderContext->SetRenderTargets( renderTargets );
    renderContext->SetPipeline( mPipeline );
    renderContext->SetViewport( windowSize );
    renderContext->SetScissor( windowSize );
    renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
    renderContext->ClearColor( swapChainColor, clearColor );
    renderContext->ClearDepth( swapChainDepth, 1 );

    TAC_CALL( renderContext->UpdateBufferSimple( mConstantBuf, cbuf, errors ) );

    renderContext->CommitShaderVariables();
    renderContext->Draw( drawArgs );

    TAC_CALL( renderContext->Execute( errors ) );
    TAC_CALL( renderDevice->Present( swapChain, errors ) );
  }

  // -----------------------------------------------------------------------------------------------

  App* App::Create()
  {
    const App::Config config
    {
      .mName { "Hello Const Buf" },
    };
    return TAC_NEW HelloConstBuf( config );
  };

} // namespace Tac

