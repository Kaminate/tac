#include "tac-std-lib/math/tac_vector3.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h" // WindowHandle
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  struct ClipSpacePosition3
  {
    explicit ClipSpacePosition3( v3 v ) : mValue( v ) {}
    explicit ClipSpacePosition3( float x, float y, float z ) : mValue{ x, y, z } {}
    v3 mValue;
  };

  struct LinearColor3
  {
    explicit LinearColor3( v3 v ) : mValue( v ) {}
    explicit LinearColor3( float x, float y, float z ) : mValue{ x, y, z } {}
    v3 mValue;
  };

  struct Vertex
  {
    ClipSpacePosition3 mPos;
    LinearColor3       mCol;
  };

  // -----------------------------------------------------------------------------------------------


  struct HelloTriangle : public App
  {
    HelloTriangle( App::Config cfg ) : App{ cfg } {}

  protected:
    void    Init( InitParams , Errors& ) override;
    //void    Update( SimUpdateParams, Errors& ) override;
    void    Render( RenderParams, Errors& ) override;
    //IState* GetGameState() override;

  private:
    void InitWindow( InitParams, Errors& );
    void InitBuffer( Errors& );
    void InitShader( Errors& );
    void InitRootSig( Errors& );

    WindowHandle           sWindowHandle;
    Render::BufferHandle   mVtxBuf;
    Render::ProgramHandle  mShaderBindless;
    Render::ProgramHandle  mShaderInputLayout;
    Render::PipelineHandle mPipelineBindless;
    Render::PipelineHandle mPipelineInputLayout;
    Render::TexFmt         mColorFormat;
    bool                   mBindless{};
  };

  void HelloTriangle::Init( InitParams initParams, Errors& errors )
  {
    const SysWindowApi* windowApi{ initParams.mWindowApi };
    mColorFormat = windowApi->GetSwapChainColorFormat();
    //WindowBackend::SysApi::mIsRendererEnabled = t; // hack

    InitWindow( initParams, errors );
    InitBuffer( errors );
    InitShader( errors );
    InitRootSig( errors );
  }

  void HelloTriangle::InitWindow( InitParams initParams, Errors& errors )
  {
    const Monitor monitor { OS::OSGetPrimaryMonitor() };
    const v2i windowSize{ monitor.mSize / 2 };
    const v2i windowPos{ ( monitor.mSize - windowSize ) / 2 };
    const WindowCreateParams windowCreateParams
    {
      .mName{ "Hello Window" },
      .mPos { windowPos },
      .mSize{ windowSize },
    };
    sWindowHandle = initParams.mWindowApi->CreateWindow( windowCreateParams, errors );
  }

  void HelloTriangle::InitShader( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::ProgramParams paramsBindless    { .mFileStem { "HelloTriangleBindless" }, };
    Render::ProgramParams paramsInputLayout { .mFileStem { "HelloTriangleInputLayout" }, };
    TAC_CALL( mShaderInputLayout = renderDevice->CreateProgram( paramsInputLayout, errors ) );
    TAC_CALL( mShaderBindless = renderDevice->CreateProgram( paramsBindless, errors ) );
  }

  void HelloTriangle::InitBuffer( Errors& errors )
  {
    // Define the geometry for a triangle.
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
        .mCol { LinearColor3{ 0.0f, 0.0f, 1.0f }}
      },
      Vertex
      {
        .mPos { ClipSpacePosition3{ 0.25f, -0.25f , 0.0f} },
        .mCol { LinearColor3{ 0.0f, 1.0f, 0.0f }}
      },
    };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::CreateBufferParams params
    {
      .mByteCount    { sizeof( triangleVertices ) },
      .mBytes        { triangleVertices },
      .mUsage        { Render::Usage::Default },
      .mBinding      { Render::Binding::ShaderResource },
      .mOptionalName { "tri verts" },
    };
    mVtxBuf = renderDevice->CreateBuffer( params, errors );
  }

  void HelloTriangle::InitRootSig( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    const Render::PipelineParams paramsBindless
    {
      .mProgram      { mShaderBindless },
      // RasterizerType         mRasterizer;
      // BlendType              mBlend;
      // DepthStencilType       mDepthStencilType;
      // SwapChainHandle               mRenderTarget;
      .mRTVColorFmts { mColorFormat },
      .mDSVDepthFmt  { Render::TexFmt::kUnknown },
    };

    const Render::PipelineParams paramsInputLayout
    {
      .mProgram      { mShaderInputLayout },
      // RasterizerType         mRasterizer;
      // BlendType              mBlend;
      // DepthStencilType       mDepthStencilType;
      // SwapChainHandle               mRenderTarget;
      .mRTVColorFmts { mColorFormat },
      .mDSVDepthFmt  { Render::TexFmt::kUnknown },
    };
    mPipelineBindless = renderDevice->CreatePipeline( paramsBindless, errors );
    mPipelineInputLayout = renderDevice->CreatePipeline( paramsInputLayout, errors );


#if 0
    // Create an empty root signature.

    const D3D12_DESCRIPTOR_RANGE1 descRange
    {
      .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
      .NumDescriptors = 1,
      .BaseShaderRegister = 0, // t0
      .RegisterSpace = 0, // space0
      .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
      .OffsetInDescriptorsFromTableStart = 0,
    };

    const Array descRanges =
    {
      descRange
    };

    const Array params =
    {
      D3D12_ROOT_PARAMETER1
      {
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        .DescriptorTable = D3D12_ROOT_DESCRIPTOR_TABLE1
        {
          .NumDescriptorRanges = (UINT)descRanges.size(),
          .pDescriptorRanges = descRanges.data()
        },
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
      },
    };

    TAC_ASSERT( myParamIndex == 0 && params.size() > myParamIndex );
    DX12SetName( m_rootSignature, "My Root Signature" );

    const DX12BuiltInputLayout inputLayout{
      VertexDeclarations
      {
        VertexDeclaration
        {
          .mAttribute = Attribute::Position,
          .mTextureFormat = Format::sv3,
          .mAlignedByteOffset = TAC_OFFSET_OF( Vertex, mPos ),
        },
        VertexDeclaration
        {
          .mAttribute = Attribute::Color,
          .mTextureFormat = Format::sv3,
          .mAlignedByteOffset = TAC_OFFSET_OF( Vertex, mCol ),
        },
      } };


    const D3D12_RASTERIZER_DESC RasterizerState
    {
      .FillMode = D3D12_FILL_MODE_SOLID,
      .CullMode = D3D12_CULL_MODE_BACK,
      .FrontCounterClockwise = true,
      .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
      .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
      .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
      .DepthClipEnable = true,
    };

    const D3D12_BLEND_DESC BlendState
    {
      .RenderTarget = 
      {
        D3D12_RENDER_TARGET_BLEND_DESC
        {
          // [x] Q: Why is BlendEnable = false? Why not just leave it out?
          //     A: You can leave it out.
#if 0
          .BlendEnable { false },
          .LogicOpEnable { false },
          .SrcBlend { D3D12_BLEND_ONE },
          .DestBlend { D3D12_BLEND_ZERO },
          .BlendOp { D3D12_BLEND_OP_ADD },
          .SrcBlendAlpha { D3D12_BLEND_ONE },
          .DestBlendAlpha { D3D12_BLEND_ZERO },
          .BlendOpAlpha { D3D12_BLEND_OP_ADD },
          .LogicOp { D3D12_LOGIC_OP_NOOP },
#endif
          .RenderTargetWriteMask { D3D12_COLOR_WRITE_ENABLE_ALL },
        },
      },
    };

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      .pRootSignature { ( ID3D12RootSignature* )m_rootSignature },
      .VS { program.GetBytecode( Render::ShaderType::Vertex ) },
      .PS { program.GetBytecode( Render::ShaderType::Fragment ) },
      .BlendState { BlendState },
      .SampleMask { UINT_MAX },
      .RasterizerState { RasterizerState },
      .DepthStencilState { D3D12_DEPTH_STENCIL_DESC{} },
      .InputLayout = sUseInputLayout
          ? ( D3D12_INPUT_LAYOUT_DESC )inputLayout
          : D3D12_INPUT_LAYOUT_DESC{},
      .PrimitiveTopologyType { D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
      .NumRenderTargets { 1 },
      .RTVFormats  { DXGIGetSwapChainFormat() },
      .SampleDesc  {.Count { 1 } },
    };
    TAC_CALL( m_device->CreateGraphicsPipelineState(
              &psoDesc,
              mPipelineState.iid(),
              mPipelineState.ppv() ));

    DX12SetName( mPipelineState, "My Pipeline State" );

#endif
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
    const v4 clearColor{ 0, 0, 1, 0 };

    renderContext->SetRenderTargets( renderTargets );

    if( mBindless )
      renderContext->SetPipeline( mPipelineBindless );
    else
      renderContext->SetPipeline( mPipelineInputLayout );

    renderContext->SetViewport( windowSize );
    renderContext->SetScissor( windowSize );
    renderContext->ClearColor( swapChainColor, clearColor );
    renderContext->ClearDepth( swapChainDepth, 1 );

    if( mBindless )
    {
    }
    else
    {
      //renderContext->SetVertexBuffer();
    }

    const Render::DrawArgs drawArgs
    {
      .mVertexCount { 3 },
    };
    renderContext->Draw( drawArgs );

    TAC_CALL( renderContext->Execute( errors ) );

#if 0

    if( sUseInputLayout )
    {
      const Array vbViews = { m_vertexBufferView };
      m_commandList->IASetVertexBuffers(0, (UINT)vbViews.size(), vbViews.data() );
    }
    else
    {
      // ...
      const Array descHeaps = { ( ID3D12DescriptorHeap* )m_srvHeap };
      m_commandList->SetDescriptorHeaps( ( UINT )descHeaps.size(), descHeaps.data() );

      // ...
      const UINT RootParameterIndex = 0;
      static_assert( RootParameterIndex == myParamIndex );
      m_commandList->SetGraphicsRootDescriptorTable( RootParameterIndex, m_srvGpuHeapStart );
    }

    m_commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    const D3D12_DRAW_ARGUMENTS drawArgs
    {
      .VertexCountPerInstance = 3,
      .InstanceCount = 1,
      .StartVertexLocation = 0,
      .StartInstanceLocation = 0,
    };
    m_commandList->DrawInstanced( drawArgs.VertexCountPerInstance,
                                  drawArgs.InstanceCount,
                                  drawArgs.StartVertexLocation,
                                  drawArgs.StartInstanceLocation );


    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( m_commandList->Close() );

    m_commandQueue->ExecuteCommandLists( ( UINT )cmdLists.size(), cmdLists.data() );

    TAC_DX12_CALL( m_swapChain->Present1( SyncInterval, PresentFlags, &params ) );
#endif
  }

  //App::IState* HelloTriangle::GetGameState() { } 


  App* App::Create()
  {
    const App::Config config
    {
      .mName { "Hello Triangle" },
    };
    return TAC_NEW HelloTriangle( config );
  };


} // namespace Tac

