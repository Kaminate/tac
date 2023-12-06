#include "tac_example_dx12_2_triangle.h" // self-inc

// todo: dx12ify
#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h"

#include "src/common/containers/tac_array.h"
#include "src/common/dataprocess/tac_text_parser.h"
#include "src/common/containers/tac_frame_vector.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/math/tac_math.h"
#include "src/common/math/tac_vector4.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_os.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"
#include "src/shell/windows/tac_win32.h"

#include <D3Dcompiler.h> // D3DCOMPILE_DEBUG

#pragma comment( lib, "d3d12.lib" ) // D3D12...

import std;

namespace Tac
{
  using namespace Render;

  void Win32Event::Init( Errors& errors )
  {
    TAC_ASSERT( !mEvent );

    // Create an event handle to use for frame synchronization.
    mEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
    TAC_RAISE_ERROR_IF( !mEvent, Win32GetLastErrorString() );
  }

  Win32Event::operator HANDLE() const { return mEvent; }

  void Win32Event::clear()
  {
    if( mEvent )
    {
      CloseHandle( mEvent );
      mEvent = nullptr;
    }
  }

  Win32Event::~Win32Event()
  {
    clear();
  }

  void Win32Event::operator = ( Win32Event&& other )
  {
    clear();
    mEvent = other.mEvent;
    other.mEvent = nullptr;
  }

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Init

  void DX12AppHelloTriangle::CreateDesktopWindow()
  {
    const OS::Monitor monitor = OS::OSGetPrimaryMonitor();
    const int s = Min( monitor.mWidth, monitor.mHeight ) / 2;
    const DesktopAppCreateWindowParams desktopParams
    {
      .mName = "DX12 Window",
      .mX = ( monitor.mWidth - s ) / 2,
      .mY = ( monitor.mHeight - s ) / 2,
      .mWidth = s,
      .mHeight = s,
    };
    hDesktopWindow = CreateTrackedWindow( desktopParams );

    DesktopAppResizeControls( hDesktopWindow );
    DesktopAppMoveControls( hDesktopWindow );
    QuitProgramOnWindowClose( hDesktopWindow );
  }

  void DX12AppHelloTriangle::EnableDebug( Errors& errors )
  {
    if( !IsDebugMode )
      return;

    PCom<ID3D12Debug> dx12debug;
    TAC_DX12_CALL( D3D12GetDebugInterface, dx12debug.iid(), dx12debug.ppv() );

    dx12debug.QueryInterface( m_debug );

    // EnableDebugLayer must be called before the device is created
    TAC_ASSERT( !m_device );
    m_debug->EnableDebugLayer();
    m_debugLayerEnabled = true;
  }

  void DX12AppHelloTriangle::CreateInfoQueue( Errors& )
  {
    if( !IsDebugMode )
      return;

    TAC_ASSERT( m_debugLayerEnabled );

    m_infoQueue = m_device.QueryInterface<ID3D12InfoQueue>( );

    TAC_ASSERT(m_infoQueue);

    // Make the application debug break when bad things happen
    m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
    m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
    m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );
  }

  void DX12AppHelloTriangle::CreateDevice( Errors& errors )
  {
    
    auto adapter = ( IDXGIAdapter* )DXGIGetBestAdapter();
    PCom< ID3D12Device > device;
    TAC_DX12_CALL( D3D12CreateDevice,
                   adapter,
                   D3D_FEATURE_LEVEL_12_1,
                   device.iid(),
                   device.ppv() );
    m_device = device.QueryInterface<ID3D12Device5>();
    DX12SetName( m_device, "Device" );
  }

  void DX12AppHelloTriangle::CreateCommandQueue( Errors& errors )
  {
    const D3D12_COMMAND_QUEUE_DESC queueDesc
    {
      // Specifies a command buffer that the GPU can execute.
      // A direct command list doesn't inherit any GPU state.
      // [ ] Q: 
      // [ ] A(?): ( As opposed to a bundle command list, which does )
      //
      // [ ] Q: What GPU state does a bundle command list inherit?
      // [ ] A: 
      //
      // [ ] Q: differnece between createcommandQueue and createCOmmandList?
      //        why does createCommandQUeue have a D3D12_COMMAND_LIST_TYPE_...?
      // [ ] A: 
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };


    TAC_DX12_CALL( m_device->CreateCommandQueue,
                   &queueDesc,
                   m_commandQueue.iid(),
                   m_commandQueue.ppv() );
    DX12SetName( (ID3D12CommandQueue*)m_commandQueue, "Command Queue" );
  }

  void DX12AppHelloTriangle::CreateRTVDescriptorHeap( Errors& errors )
  {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors
    // Descriptors are the primary unit of binding for a single resource in D3D12.



    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
    // A descriptor heap is a collection of contiguous allocations of descriptors,
    // one allocation for every descriptor.
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = bufferCount,
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap,
                   &desc,
                   m_rtvHeap.iid(),
                   m_rtvHeap.ppv() );
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
    m_rtvHeapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloTriangle::CreateCommandAllocator( Errors& errors )
  {
    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   m_commandAllocator.iid(),
                   m_commandAllocator.ppv()  );
  }

  void DX12AppHelloTriangle::CreateCommandList( Errors& errors )
  {
    // Create the command list (CreateCommandList1 creates it in a closed state).
    PCom< ID3D12CommandList > commandList;
    TAC_DX12_CALL( m_device->CreateCommandList1,
                   0,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   D3D12_COMMAND_LIST_FLAG_NONE,
                   m_commandList.iid(),
                   m_commandList.ppv() );

    commandList.QueryInterface( m_commandList );
    TAC_ASSERT(m_commandList);
  }

  void DX12AppHelloTriangle::CreateFence( Errors& errors )
  {
    // Create synchronization objects.

    const UINT64 initialVal = 0;

    PCom< ID3D12Fence > fence;
    TAC_DX12_CALL( m_device->CreateFence,
                   initialVal,
                   D3D12_FENCE_FLAG_NONE,
                   fence.iid(),
                   fence.ppv() );

    fence.QueryInterface(m_fence);

    m_fenceValue = 1;

    TAC_CALL( m_fenceEvent.Init, errors );
  }


  void DX12AppHelloTriangle::CreateRootSignature( Errors& errors )
  {
    // Create an empty root signature.

    const D3D12_ROOT_SIGNATURE_DESC1 Desc_1_1
    {
      .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc
    {
      .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
      .Desc_1_1 = Desc_1_1,
    };

    PCom<ID3DBlob> blob;
    PCom<ID3DBlob> blobErr;

    // cursed or based?
    TAC_RAISE_ERROR_IF( const HRESULT hr =
                        D3D12SerializeVersionedRootSignature(
                        &desc,
                        blob.CreateAddress(),
                        blobErr.CreateAddress() ); FAILED( hr ),
                        String() +
                        "Failed to serialize root signature! "
                        "Blob = " + ( const char* )blobErr->GetBufferPointer() + ", "
                        "HRESULT = " + DX12_HRESULT_ToString( hr ) );

    TAC_DX12_CALL( m_device->CreateRootSignature,
                   0,
                   blob->GetBufferPointer(),
                   blob->GetBufferSize(),
                   m_rootSignature.iid(),
                   m_rootSignature.ppv() );

  }

  struct DX12ShaderCompileFromStringInput
  {
    StringView  mPreprocessedShader;
    const char* mEntryPoint;
    const char* mTarget;
  };

  struct DX12ShaderCompileOutput
  {
    PCom< ID3DBlob >      mBlob;
    D3D12_SHADER_BYTECODE mByteCode;
  };

  String DX12PreprocessShader( StringView shader )
  {
    String newShader;

    ParseData parse( shader );
    while( parse )
    {
      const StringView line = parse.EatRestOfLine();

      newShader += PreprocessShaderSemanticName( line );
      newShader += '\n';
    }

    return newShader;
  }

  DX12ShaderCompileOutput DX12CompileShaderFromString(
    const DX12ShaderCompileFromStringInput& input,
    Errors& errors )
  {
    PCom< ID3DBlob > blob;
    PCom< ID3DBlob > blobErr;

    const UINT flags = IsDebugMode ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION : 0;

    
    // todo: add an assert that if we are using d3dcompile, shader model is 5_1 or below

    TAC_RAISE_ERROR_IF_RETURN(
      const HRESULT hr = D3DCompile(
      input.mPreprocessedShader.data(),
      input.mPreprocessedShader.size(),
      nullptr,
      nullptr,
      nullptr,
      input.mEntryPoint,
      input.mTarget,
      flags,
      0,
      blob.CreateAddress(),
      blobErr.CreateAddress() );
    FAILED( hr ),
      String() +
      "DX12 failed to compile shader, "
      "error blob: " + ( const char* )blobErr->GetBufferPointer() + ", "
      "hr: " + DX12_HRESULT_ToString( hr ) + ", "
      "shader: \n" + input.mPreprocessedShader,
      DX12ShaderCompileOutput{} );

    return
    {
      .mBlob = blob,
      .mByteCode = D3D12_SHADER_BYTECODE
      {
        .pShaderBytecode = blob->GetBufferPointer(),
        .BytecodeLength = blob->GetBufferSize(),
      },
    };
  }

  struct Vertex
  {
    v3 mPos;
    v4 mColor;
  };

  struct DX12InputLayoutBuilder
  {
    DX12InputLayoutBuilder() = default;
    DX12InputLayoutBuilder( const VertexDeclarations& vtxDecls )
    {
      for( const auto& decl : vtxDecls )
      {
        decl.mAlignedByteOffset;
        decl.mAttribute;
        decl.mTextureFormat;
      }
    }

    void Append( const char* semanticName,
                 DXGI_FORMAT fmt,
                 UINT offset = D3D12_APPEND_ALIGNED_ELEMENT)
    {
      const D3D12_INPUT_ELEMENT_DESC desc
      {
        .SemanticName = semanticName,
        .Format = fmt,
        .AlignedByteOffset = offset,
      };

      mElementDescs.push_back( desc );
    }

    D3D12_INPUT_LAYOUT_DESC GetLayoutDesc()
    {
      UpdateLayoutDesc();
      return mLayoutDesc;
    }

  private:
    void UpdateLayoutDesc()
    {
      mLayoutDesc = D3D12_INPUT_LAYOUT_DESC
      {
        .pInputElementDescs = mElementDescs.data(),
        .NumElements = ( UINT )mElementDescs.size(),
      };
    }
    FixedVector< D3D12_INPUT_ELEMENT_DESC, 10 > mElementDescs;
    D3D12_INPUT_LAYOUT_DESC                     mLayoutDesc;
  };

  static D3D_SHADER_MODEL GetHighestShaderModel( ID3D12Device* device )
  {
    const D3D_SHADER_MODEL lowestShaderModel = D3D_SHADER_MODEL_5_1;
    D3D_SHADER_MODEL highestShaderModel = D3D_HIGHEST_SHADER_MODEL;
    for( ;; )
    {
      D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{ highestShaderModel };
      if( SUCCEEDED( device->CheckFeatureSupport( D3D12_FEATURE_SHADER_MODEL,
          &shaderModel,
          sizeof( D3D12_FEATURE_DATA_SHADER_MODEL ) ) ) )
        break;
      highestShaderModel = D3D_SHADER_MODEL(highestShaderModel - 1);
      if( highestShaderModel == lowestShaderModel )
        break;
    }

    return highestShaderModel;
  }

  String GetTarget( const char* shader, D3D_SHADER_MODEL model )
  {
    //char buf[ 10 ];

    std::stringstream ss;
    ss << std::hex << (int) model;
    auto hexStr = ss.str();

    //std::itoa( (int) model, buf, 16 );
    //model.D3D_HIGHEST_SHADER_MODEL;

    TAC_ASSERT( hexStr.size() == 2 );

    String s;
    s += shader;
    s += '_';
    s += hexStr[0];
    s += '_';
    s += hexStr[1];
    return s;
  }

  void DX12AppHelloTriangle::CreatePipelineState( Errors& errors )
  {
    const AssetPathStringView shaderAssetPath = "assets/hlsl/DX12HelloTriangle.hlsl";

    const String shaderStrRaw = TAC_CALL( LoadAssetPath, shaderAssetPath, errors );
    const String shaderStrProcessed = DX12PreprocessShader( shaderStrRaw );

    D3D_SHADER_MODEL shaderModel = D3D_SHADER_MODEL_6_6;

    const DX12ShaderCompileFromStringInput vsInput
    {
      .mPreprocessedShader = shaderStrProcessed,
      .mEntryPoint = "VSMain",
      .mTarget = "vs_5_1",// GetTarget( "vs", shaderModel ), //"vs_6_6",
    };

    const DX12ShaderCompileFromStringInput psInput
    {
      .mPreprocessedShader = shaderStrProcessed,
      .mEntryPoint = "PSMain",
      .mTarget = "ps_6_4",// GetTarget( "ps", shaderModel ), // "ps_6_6",
    };




    D3D_SHADER_MODEL highestShaderModel = GetHighestShaderModel( (ID3D12Device*)m_device );
    TAC_ASSERT( highestShaderModel >= shaderModel );



    // so use dxc, and assert that the shader model is above 5_1
    TAC_ASSERT_CRITICAL( 
    "the d3dcompiler.dll and D3Dcompiler.h D3DCompile() is only valid for shader models up to 5_1,"
    "for shader model 6_0+, need dxc.exe dxcompiler.dll" );

    auto [ vsBlob, vsBytecode ] = TAC_CALL( DX12CompileShaderFromString, vsInput, errors );
    auto [ psBlob, psBytecode ] = TAC_CALL( DX12CompileShaderFromString, psInput, errors );


    PCom< ID3D12ShaderReflection > refl;
    TAC_DX12_CALL( D3DReflect,
                   vsBytecode.pShaderBytecode,
                   vsBytecode.BytecodeLength,
                   refl.iid(),
                   refl.ppv() );

    D3D12_SHADER_DESC shaderDesc;
    TAC_DX12_CALL( refl->GetDesc, &shaderDesc );

    for( UINT ParameterIndex = 0;
         ParameterIndex < shaderDesc.InputParameters;
         ParameterIndex++ )
    {
      D3D12_SIGNATURE_PARAMETER_DESC desc;
      TAC_DX12_CALL(refl->GetInputParameterDesc,ParameterIndex, &desc );

      desc.SemanticName;
      ++asdf;
    }
    

    //const VertexDeclarations vtxDecls
    //{
    //  {
    //    .mAttribute = Attribute::Position,
    //    .mTextureFormat = Format::sv3,
    //    .mAlignedByteOffset = TAC_OFFSET_OF( Vertex, mPos ),
    //  },
    //  {
    //    .mAttribute = Attribute::Color,
    //    .mTextureFormat = Format::sv4,
    //    .mAlignedByteOffset = TAC_OFFSET_OF( Vertex, mColor ),
    //  },
    //};

  //{
  //  Attribute mAttribute = Attribute::Count;
  //  Format    mTextureFormat;

  //  //        Offset of the variable from the vertex buffer
  //  //        ie: OffsetOf( MyVertexType, mPosition)
  //  int       mAlignedByteOffset = 0;
  //};
  //  };
    //Render::inputlay

    DX12InputLayoutBuilder inputLayout;
    inputLayout.Append( "POOP", DXGI_FORMAT_R32G32B32_FLOAT );
    inputLayout.Append( "POOP", DXGI_FORMAT_R32G32B32A32_FLOAT );

    const D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = inputLayout.GetLayoutDesc();

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
        // [ ] Q: ??? why enable = false?
          .BlendEnable = false,
          .LogicOpEnable = false,
          .SrcBlend = D3D12_BLEND_ONE,
          .DestBlend = D3D12_BLEND_ZERO,
          .BlendOp = D3D12_BLEND_OP_ADD,
          .SrcBlendAlpha= D3D12_BLEND_ONE,
          .DestBlendAlpha= D3D12_BLEND_ZERO,
          .BlendOpAlpha= D3D12_BLEND_OP_ADD,
          .LogicOp = D3D12_LOGIC_OP_NOOP,
          .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        },
      },
    };

    const D3D12_DEPTH_STENCIL_DESC DepthStencilState {};

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      .pRootSignature = ( ID3D12RootSignature* )m_rootSignature,
      .VS = vsBytecode,
      .PS = psBytecode,
      .BlendState = BlendState,
      .SampleMask = UINT_MAX,
      .RasterizerState = RasterizerState,
      .DepthStencilState = DepthStencilState,
      //.InputLayout = inputLayoutDesc,
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = { DXGIGetSwapChainFormat() },
      .SampleDesc = { .Count = 1 },
    };
    TAC_CALL( m_device->CreateGraphicsPipelineState,
              &psoDesc,
              mPipelineState.iid(),
              mPipelineState.ppv() );

    ++asdf;

  }

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Update

  void DX12AppHelloTriangle::DX12CreateSwapChain( Errors& errors )
  {
    const DesktopWindowState* state = GetDesktopWindowState( hDesktopWindow );
    const auto hwnd = ( HWND )state->mNativeWindowHandle;
    if( !hwnd )
      return;

    TAC_ASSERT( m_commandQueue );

    const SwapChainCreateInfo scInfo
    {
      .mHwnd = hwnd,
      .mDevice = (IUnknown*)m_commandQueue, // swap chain can force flush the queue
      .mBufferCount = bufferCount,
      .mWidth = state->mWidth,
      .mHeight = state->mHeight,
    };
    m_swapChain = TAC_CALL( DXGICreateSwapChain, scInfo, errors );
    TAC_CALL( m_swapChain->GetDesc1, &m_swapChainDesc );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::GetRenderTargetDescriptorHandle( int i ) const
  {
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeapStart;
    rtvHandle.ptr += i * m_rtvDescriptorSize;
    return rtvHandle;
  }

  void DX12AppHelloTriangle::CreateRenderTargetViews( Errors& errors )
  {
    TAC_ASSERT( m_swapChain );
    TAC_ASSERT( m_device );

    // Create a RTV for each frame.
    for( UINT i = 0; i < bufferCount; i++ )
    {
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetDescriptorHandle( i );
      PCom< ID3D12Resource >& renderTarget = m_renderTargets[ i ];
      TAC_DX12_CALL( m_swapChain->GetBuffer, i, renderTarget.iid(), renderTarget.ppv() );
      m_device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      // the render target resource is created in a state that is ready to be displayed on screen
      m_renderTargetStates[i] = D3D12_RESOURCE_STATE_PRESENT;
    }
  }

  void DX12AppHelloTriangle::TransitionRenderTarget( const int iRT,
                                                   const D3D12_RESOURCE_STATES targetState )
  {
    ID3D12Resource* rtResource = (ID3D12Resource*)m_renderTargets[ iRT ];
    TAC_ASSERT(rtResource );

    const D3D12_RESOURCE_STATES before = m_renderTargetStates[ iRT ];
    TAC_ASSERT( before != targetState );

    const D3D12_RESOURCE_BARRIER barrier
    {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = D3D12_RESOURCE_TRANSITION_BARRIER
      {
        .pResource = rtResource,
        .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        .StateBefore = before,
        .StateAfter = targetState,
      },
    };

    m_renderTargetStates[ iRT ] = targetState;

    ResourceBarrier( barrier );
  }

  void DX12AppHelloTriangle::ResourceBarrier( const D3D12_RESOURCE_BARRIER& barrier )
  {
    // Resource barriers are used to manage resource transitions.

    // ID3D12CommandList::ResourceBarrier
    // - Notifies the driver that it needs to synchronize multiple accesses to resources.
    //
    const Array barriers = { barrier };
    const UINT rtN = ( UINT )barriers.size();
    const D3D12_RESOURCE_BARRIER* rts = barriers.data();
    m_commandList->ResourceBarrier( rtN, rts );
  }

  void DX12AppHelloTriangle::PopulateCommandList( Errors& errors )
  {
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    //
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset
    // ID3D12CommandAllocator::Reset
    //   Indicates to re-use the memory that is associated with the command allocator.
    //   From this call to Reset, the runtime and driver determine that the GPU is no longer
    //   executing any command lists that have recorded commands with the command allocator.
    TAC_DX12_CALL( m_commandAllocator->Reset );

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    //
    // ID3D12GraphicsCommandList::Reset
    //   you can re-use command list tracking structures without any allocations
    //   you can call Reset while the command list is still being executed
    //   you can submit a cmd list, reset it, and reuse the allocated memory for another cmd list
    TAC_DX12_CALL( m_commandList->Reset,
                   ( ID3D12CommandAllocator* )m_commandAllocator,
                   ( ID3D12PipelineState* )m_pipelineState );

    // Indicate that the back buffer will be used as a render target.
    TransitionRenderTarget( m_frameIndex, D3D12_RESOURCE_STATE_RENDER_TARGET );

    ClearRenderTargetView();

    // Indicate that the back buffer will now be used to present.
    //
    // When a back buffer is presented, it must be in the D3D12_RESOURCE_STATE_PRESENT state.
    // If IDXGISwapChain1::Present1 is called on a resource which is not in the PRESENT state,
    // a debug layer warning will be emitted.
    TransitionRenderTarget( m_frameIndex, D3D12_RESOURCE_STATE_PRESENT );

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( m_commandList->Close );
  }

  void DX12AppHelloTriangle::ClearRenderTargetView()
  {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRenderTargetDescriptorHandle( m_frameIndex );

    const double speed = 3;
    const auto t = ( float )Sin( ShellGetElapsedSeconds() * speed ) * 0.5f + 0.5f;

    // Record commands.
    const float clearColor[] = { t, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView( rtvHandle, clearColor, 0, nullptr );
  }

  void DX12AppHelloTriangle::ExecuteCommandLists()
  {
    const Array lists =
    {
      ( ID3D12CommandList* )( ID3D12GraphicsCommandList* )m_commandList
    };

    // Submits an array of command lists for execution.
    m_commandQueue->ExecuteCommandLists( ( UINT )lists.size(), lists.data() );
  }

  void DX12AppHelloTriangle::SwapChainPresent( Errors& errors )
  {
    // Present the frame.

    // [x] Q: What is the frame?
    //        Is it the current frame?
    //        The last frame?
    //     A: From experimentation, you can consider the 'frame' to refer to
    //        the render target indexed by the swap chain's current backbuffer index.
    //        (m_renderTargets[m_swapChain->GetCurrentBackBufferIndex())
    //        
    //        This render target is in D3D12_RESOURCE_STATE_PRESENT,
    //        And after calling swap chain IDXGISwapChain::Present(), the next call to 
    //        IDXGISwapChain::GetCurrentBackBufferIndex() will be 1 higher
    //
    //        I think the swap chain flushes the command queue before rendering,
    //        so the frame being presented is the one that we just called ExecuteCommandLists() on

    // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model
    // In the flip model, all back buffers are shared with the Desktop Window Manager (DWM)
    // 1.	The app updates its frame (Write)
    // 2. Direct3D runtime passes the app surface to DWM
    // 3. DWM renders the app surface onto screen( Read, Write )

    TAC_ASSERT( m_swapChainDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL );

    const DXGI_PRESENT_PARAMETERS params{};

    // For the flip model (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL), values are:
    //   0   - Cancel the remaining time on the previously presented frame
    //         and discard this frame if a newer frame is queued.
    //   1-4 - Synchronize presentation for at least n vertical blanks.
    const UINT SyncInterval = 1;
    const UINT PresentFlags = 0;

    // I think this technically adds a frame onto the present queue
    TAC_DX12_CALL( m_swapChain->Present1, SyncInterval, PresentFlags, &params );
  }

  void DX12AppHelloTriangle::WaitForPreviousFrame( Errors& errors )
  {
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // ID3D12CommandQueue::Signal
    // - Updates a fence to a specified value.
    // ^ I think this adds a signal command to the queue, which basically delay sets the fence
    //   to a value
    // ^ A command queue is a not a command list, so this maybe adds a signal to be fired
    //   in between execution of command lists?

    // Signal and increment the fence value.

    // (first call)
    //   m_fence was initialized with value 0,
    //   m_fenceValue was initialized to 1
    //
    //   so we signal m_fence(0) with m_fenceValue(1)
    //   increment m_fenceValue(2)
    //

    const UINT64 signalValue = m_fenceValue;

    // Use this method to set a fence value from the GPU side
    // [ ] Q: ^ ???
    TAC_DX12_CALL( m_commandQueue->Signal, (ID3D12Fence*)m_fence, signalValue );

    m_fenceValue++;

    // Experimentally, m_fence->GetCompletedValue() doesn't hit the signalled value until

    // ID3D12Fence::GetCompletedValue
    // - Gets the current value of the fence.
    //
    // Wait until the previous frame is finished.

    // I think this if statement is used because the alternative
    // would be while( m_fence->GetCompletedValue() != fence ) { TAC_NO_OP; }
    const UINT64 curValue = m_fence->GetCompletedValue();
    if( curValue < signalValue )
    {
      // m_fenceEvent is only ever used in this scope 

      // ID3D12Fence::SetEventOnCompletion
      // - Specifies an event that's raised when the fence reaches a certain value.
      //
      // the event will be 'complete' when it reaches the specified value.
      // This value is set by the cmdqueue::Signal
      TAC_DX12_CALL( m_fence->SetEventOnCompletion, signalValue, (HANDLE)m_fenceEvent );
      WaitForSingleObject( (HANDLE)m_fenceEvent, INFINITE );
    }

    // I think this would be the same if it were called after IDXGISwapChain::Present instead
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  }


  // -----------------------------------------------------------------------------------------------

  DX12AppHelloTriangle::DX12AppHelloTriangle( const Config& cfg ) : App( cfg ) {}

  void DX12AppHelloTriangle::Init( Errors& errors )
  {
    CreateDesktopWindow();

    TAC_CALL( DXGIInit, errors );
    TAC_CALL( EnableDebug, errors );
    TAC_CALL( CreateDevice, errors );
    TAC_CALL( CreateInfoQueue, errors );
    TAC_CALL( CreateCommandQueue, errors );
    TAC_CALL( CreateRTVDescriptorHeap, errors );
    TAC_CALL( CreateCommandAllocator, errors );
    TAC_CALL( CreateCommandList, errors );
    TAC_CALL( CreateFence, errors );
    TAC_CALL( CreateRootSignature, errors );
    TAC_CALL( CreatePipelineState, errors );

  }

  void DX12AppHelloTriangle::Update( Errors& errors )
  {

    if( !GetDesktopWindowNativeHandle( hDesktopWindow ) )
      return;

    if( !m_swapChain )
    {
      TAC_CALL( DX12CreateSwapChain, errors );
      TAC_CALL( CreateRenderTargetViews, errors );
    }

    // Record all the commands we need to render the scene into the command list.
    TAC_CALL( PopulateCommandList, errors );

    ExecuteCommandLists();

    TAC_CALL( SwapChainPresent, errors );


    TAC_CALL( WaitForPreviousFrame, errors );

    ++asdf;
  }

  void DX12AppHelloTriangle::Uninit( Errors& errors )
  {
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    TAC_CALL( WaitForPreviousFrame, errors );

    DXGIUninit();
  }

  App* App::Create()
  {
    const App::Config config
    {
      .mName = "DX12 Hello Window",
      .mDisableRenderer = true,
    };
    return TAC_NEW DX12AppHelloTriangle( config );
  };


} // namespace Tac

