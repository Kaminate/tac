
#include "tac_example_dx12_2_triangle.h" // self-inc

// todo: dx12ify
#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h"

#include "tac_example_dx12_2_dxc.h"
#include "src/common/containers/tac_array.h"
#include "src/common/dataprocess/tac_text_parser.h"
#include "src/common/containers/tac_frame_vector.h"
#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/memory/tac_frame_memory.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/math/tac_math.h"
#include "src/common/math/tac_vector4.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_os.h"
#include "src/common/shell/tac_shell.h"
#include "src/shell/tac_desktop_app.h"
#include "src/shell/tac_desktop_window_settings_tracker.h"
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"
#include "src/shell/windows/tac_win32.h"

// set to true to use IASetVertexBuffers with an input layout
// set to false to use bindless through a descriptor table
// ( technically both use a vertex buffer, but only one uses input layout )
static const bool sUseInputLayout = false;

#pragma comment( lib, "d3d12.lib" ) // D3D12...

const UINT myParamIndex = 0;

import std;

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  struct ClipSpacePosition3
  {
    explicit ClipSpacePosition3( v3 v ) : mValue( v ) {}
    explicit ClipSpacePosition3(float x, float y, float z) : mValue{ x,y,z } {}
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
    LinearColor3 mCol;
  };

  // -----------------------------------------------------------------------------------------------

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
    if constexpr( !IsDebugMode )
      return;

    PCom<ID3D12Debug> dx12debug;
    TAC_DX12_CALL( D3D12GetDebugInterface, dx12debug.iid(), dx12debug.ppv() );

    dx12debug.QueryInterface( m_debug );

    // EnableDebugLayer must be called before the device is created
    TAC_ASSERT( !m_device );
    m_debug->EnableDebugLayer();
    m_debugLayerEnabled = true;
  }

  void  MyD3D12MessageFunc( D3D12_MESSAGE_CATEGORY Category,
                            D3D12_MESSAGE_SEVERITY Severity,
                            D3D12_MESSAGE_ID ID,
                            LPCSTR pDescription,
                            void* pContext )
  {
    OS::OSDebugBreak();
  }

  void DX12AppHelloTriangle::CreateInfoQueue( Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    TAC_ASSERT( m_debugLayerEnabled );

    m_device.QueryInterface( m_infoQueue );
    TAC_ASSERT(m_infoQueue);

    // Make the application debug break when bad things happen
    TAC_DX12_CALL(m_infoQueue->SetBreakOnSeverity, D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
    TAC_DX12_CALL(m_infoQueue->SetBreakOnSeverity, D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
    TAC_DX12_CALL(m_infoQueue->SetBreakOnSeverity, D3D12_MESSAGE_SEVERITY_WARNING, TRUE );

    // First available in Windows 10 Release Preview build 20236,
    // But as of 2023-12-11 not available on my machine :(
    if( auto infoQueue1 = m_infoQueue.QueryInterface<ID3D12InfoQueue1>() )
    {
      const D3D12MessageFunc CallbackFunc = MyD3D12MessageFunc;
      const D3D12_MESSAGE_CALLBACK_FLAGS CallbackFilterFlags = D3D12_MESSAGE_CALLBACK_FLAG_NONE;
      void* pContext = this;
      DWORD pCallbackCookie;

      TAC_DX12_CALL( infoQueue1->RegisterMessageCallback,
                     CallbackFunc,
                     CallbackFilterFlags,
                     pContext,
                     &pCallbackCookie );
    }
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

    if constexpr( IsDebugMode )
    {
      m_device.QueryInterface( m_debugDevice );
      TAC_ASSERT( m_debugDevice );
    }

    InitDescriptorSizes();
  }

  void DX12AppHelloTriangle::InitDescriptorSizes()
  {
    for( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
      m_descriptorSizes[ i ]
      = m_device->GetDescriptorHandleIncrementSize( ( D3D12_DESCRIPTOR_HEAP_TYPE )i );
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

      // This command queue manages direct command lists (direct for graphics rendering)
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    TAC_DX12_CALL( m_device->CreateCommandQueue,
                   &queueDesc,
                   m_commandQueue.iid(),
                   m_commandQueue.ppv() );
    DX12SetName( m_commandQueue, "Command Queue" );
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
    m_rtvCpuHeapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_rtvGpuHeapStart = m_rtvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloTriangle::CreateSRVDescriptorHeap( Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = 1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap,
                   &desc,
                   m_srvHeap.iid(),
                   m_srvHeap.ppv() );
    m_srvCpuHeapStart = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvGpuHeapStart = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloTriangle::CreateSRV( Errors& errors )
  {
    TAC_ASSERT( m_vertexBuffer );

    // srv --> byteaddressbuffer
    // uav --> rwbyteaddressbuffer

    const D3D12_SHADER_RESOURCE_VIEW_DESC Desc
    {
      .Format = DXGI_FORMAT_R32_TYPELESS, // for byteaddressbuffer
      .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, // swizzling?
      .Buffer = D3D12_BUFFER_SRV
      {
        .FirstElement = 0,
        .NumElements = m_vertexBufferView.SizeInBytes / ( UINT )sizeof( std::uint32_t ),
        .Flags = D3D12_BUFFER_SRV_FLAG_RAW, // for byteaddressbuffer
      },
    };


    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = GetSRVCpuDescHandle( 0 );
    
    m_device->CreateShaderResourceView( ( ID3D12Resource* )m_vertexBuffer,
                                        &Desc,
                                        DestDescriptor );

  }

  void DX12AppHelloTriangle::CreateCommandAllocator( Errors& errors )
  {
    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   m_commandAllocator.iid(),
                   m_commandAllocator.ppv()  );
    DX12SetName( m_commandAllocator, "My Command Allocator");
  }

  void DX12AppHelloTriangle::CreateCommandList( Errors& errors )
  {
    // Create the command list
    // (CreateCommandList1 creates it in a closed state, as opposed to
    //  CreateCommandList, which creates in a open state).
    PCom< ID3D12CommandList > commandList;
    TAC_DX12_CALL( m_device->CreateCommandList1,
                   0,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   D3D12_COMMAND_LIST_FLAG_NONE,
                   commandList.iid(),
                   commandList.ppv() );
    TAC_ASSERT(commandList);
    commandList.QueryInterface( m_commandList );
    TAC_ASSERT(m_commandList);
    DX12SetName( m_commandList, "My Command List");
  }

  void DX12AppHelloTriangle::CreateVertexBuffer( Errors& errors )
  {
    const float m_aspectRatio = (float)m_swapChainDesc.Width / (float)m_swapChainDesc.Height; 

    // Define the geometry for a triangle.
    const Vertex triangleVertices[] =
    {
      Vertex
      {
        .mPos = ClipSpacePosition3{0.0f, 0.25f * m_aspectRatio, 0.0f},
        .mCol = LinearColor3{ 1.0f, 0.0f, 0.0f}
      },
      Vertex
      {
        .mPos = ClipSpacePosition3{ -0.25f, -0.25f * m_aspectRatio, 0.0f},
        .mCol = LinearColor3{ 0.0f, 0.0f, 1.0f}
      },
      Vertex
      {
        .mPos = ClipSpacePosition3{ 0.25f, -0.25f * m_aspectRatio, 0.0f},
        .mCol = LinearColor3{ 0.0f, 1.0f, 0.0f}
      },
    };

    const UINT vertexBufferSize = sizeof( triangleVertices );


    const D3D12_HEAP_PROPERTIES uploadHeapProps
    {
      .Type = D3D12_HEAP_TYPE_UPLOAD,
      .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
      .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    };

    const D3D12_RESOURCE_DESC resourceDesc
    {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Alignment = 0,
      .Width = vertexBufferSize,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc
      {
        .Count = 1,
        .Quality = 0,
      },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    // must be null for buffer
    const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr;

    // D3D12_RESOURCE_STATE_GENERIC_READ
    //   An OR'd combination of other read-state bits.
    //   The required starting state for an upload heap
    const D3D12_RESOURCE_STATES uploadHeapResourceStates = D3D12_RESOURCE_STATE_GENERIC_READ;
    TAC_CALL( m_device->CreateCommittedResource,
              &uploadHeapProps,
              D3D12_HEAP_FLAG_NONE,
              &resourceDesc,
              uploadHeapResourceStates,
              pOptimizedClearValue,
              m_vertexBufferUploadHeap.iid(),
              m_vertexBufferUploadHeap.ppv() );

    const D3D12_HEAP_PROPERTIES defaultHeapProps
    {
      .Type = D3D12_HEAP_TYPE_DEFAULT,
      .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
      .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
    };

    // Creates both a resource and an implicit heap,
    // such that the heap is big enough to contain the entire resource,
    // and the resource is mapped to the heap.
    TAC_CALL( m_device->CreateCommittedResource,
      &defaultHeapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST, // we want to copy into here from the uplaod buffer
      pOptimizedClearValue,
      m_vertexBuffer.iid(),
      m_vertexBuffer.ppv() );

    DX12SetName( m_vertexBuffer, "vertexes");

    // Copy the triangle data to the vertex buffer.
    const D3D12_RANGE readRange{}; // not reading from CPU
    void* pVertexDataBegin;
    TAC_DX12_CALL( m_vertexBuffer->Map, 0, &readRange, &pVertexDataBegin );
    MemCpy( pVertexDataBegin, triangleVertices, vertexBufferSize );
    m_vertexBuffer->Unmap( 0, nullptr );


    m_commandList->ResourceBarrier;

    // Initialize the vertex buffer view.
    m_vertexBufferView = D3D12_VERTEX_BUFFER_VIEW 
    {
      .BufferLocation = m_vertexBuffer->GetGPUVirtualAddress(),
      .SizeInBytes = vertexBufferSize,
      .StrideInBytes = sizeof( Vertex ),
    };

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
    DX12SetName( fence, "fence" );

    m_fenceValue = 1;

    TAC_CALL( m_fenceEvent.Init, errors );
  }


  void DX12AppHelloTriangle::CreateRootSignature( Errors& errors )
  {
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

    const D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = sUseInputLayout
      ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
      : D3D12_ROOT_SIGNATURE_FLAG_NONE;

    const D3D12_ROOT_SIGNATURE_DESC1 Desc_1_1
    {
      .NumParameters = (UINT)params.size(),
      .pParameters = params.data(),


       // The app is opting in to using the Input Assembler
       // ( requiring an input layout that defines a set of vertex buffer bindings ).
       // Omitting this flag can result in one root argument space being saved on some hardware.
       // Omit this flag if the Input Assembler is not required, though the optimization is minor.
      .Flags = rootSigFlags,
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

    DX12SetName( m_rootSignature, "My Root Signature" );
  }


  static String DX12PreprocessShader( StringView shader )
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

  struct DX12BuiltInputLayout : public D3D12_INPUT_LAYOUT_DESC
  {
    DX12BuiltInputLayout( const VertexDeclarations& vtxDecls )
    {
      const int n = vtxDecls.size();
      mElementDescs.resize(n );
      for( int i = 0; i < n; ++i )
      {
        const auto& decl = vtxDecls[ i ];
        mElementDescs[ i ] = D3D12_INPUT_ELEMENT_DESC
        {
          .SemanticName = GetSemanticName( decl.mAttribute ),
          .Format = GetDXGIFormatTexture( decl.mTextureFormat ),
          .AlignedByteOffset = (UINT)decl.mAlignedByteOffset,
        };
      }

      *( D3D12_INPUT_LAYOUT_DESC* )this = D3D12_INPUT_LAYOUT_DESC
      {
        .pInputElementDescs = mElementDescs.data(),
        .NumElements = (UINT)n,
      };
    }
    FixedVector< D3D12_INPUT_ELEMENT_DESC, 10 > mElementDescs;
  };

  static D3D_SHADER_MODEL GetHighestShaderModel( ID3D12Device* device )
  {
    const D3D_SHADER_MODEL lowestDefined = D3D_SHADER_MODEL_5_1;
    const D3D_SHADER_MODEL highestDefined = D3D_SHADER_MODEL_6_7; // D3D_HIGHEST_SHADER_MODEL undefined?;
    for( D3D_SHADER_MODEL shaderModel = highestDefined; 
         shaderModel >= lowestDefined;
         shaderModel = D3D_SHADER_MODEL( shaderModel - 1 ) )
    {
      // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model
      //   After the function completes successfully, the HighestShaderModel field contains the
      //   highest shader model that is both supported by the device and no higher than the
      //   shader model passed in.
      TAC_NOT_CONST D3D12_FEATURE_DATA_SHADER_MODEL featureData{ shaderModel };
      if( SUCCEEDED( device->CheckFeatureSupport(
        D3D12_FEATURE_SHADER_MODEL,
        &featureData,
        sizeof(D3D12_FEATURE_DATA_SHADER_MODEL) ) ) )
        
        // For some godforsaken fucking reason, this isn't the same as shaderModel
        return featureData.HighestShaderModel;
    }

    return lowestDefined;
  }


  void DX12AppHelloTriangle::CreatePipelineState( Errors& errors )
  {
    const AssetPathStringView shaderAssetPath = sUseInputLayout
      ? "assets/hlsl/DX12HelloTriangle.hlsl"
      : "assets/hlsl/DX12HelloTriangleBindless.hlsl";

    const String shaderStrRaw = TAC_CALL( LoadAssetPath, shaderAssetPath, errors );
    const String shaderStrProcessed = DX12PreprocessShader( shaderStrRaw );

    const D3D_SHADER_MODEL shaderModel = D3D_SHADER_MODEL_6_5;
    const D3D_SHADER_MODEL highestShaderModel = GetHighestShaderModel( (ID3D12Device*)m_device );
    TAC_ASSERT( shaderModel <= highestShaderModel );

    const DX12ShaderCompileFromStringInput vsInput
    {
      .mShaderAssetPath = shaderAssetPath,
      .mPreprocessedShader = shaderStrProcessed,
      .mEntryPoint = "VSMain",
      .mType = ShaderType::Vertex,
      .mShaderModel = shaderModel,
    };

    const DX12ShaderCompileFromStringInput psInput
    {
      .mShaderAssetPath = shaderAssetPath,
      .mPreprocessedShader = shaderStrProcessed,
      .mEntryPoint = "PSMain",
      .mType = ShaderType::Fragment,
      .mShaderModel = shaderModel,
    };


    auto [ vsBlob, vsBytecode ] = TAC_CALL( DX12CompileShaderDXC, vsInput, errors );
    auto [ psBlob, psBytecode ] = TAC_CALL( DX12CompileShaderDXC, psInput, errors );
    

    const DX12BuiltInputLayout inputLayout(
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
      } );


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

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      .pRootSignature = ( ID3D12RootSignature* )m_rootSignature,
      .VS = vsBytecode,
      .PS = psBytecode,
      .BlendState = BlendState,
      .SampleMask = UINT_MAX,
      .RasterizerState = RasterizerState,
      .DepthStencilState = D3D12_DEPTH_STENCIL_DESC{},
      .InputLayout = sUseInputLayout
          ? ( D3D12_INPUT_LAYOUT_DESC )inputLayout
          : D3D12_INPUT_LAYOUT_DESC{},
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = { DXGIGetSwapChainFormat() },
      .SampleDesc = { .Count = 1 },
    };
    TAC_CALL( m_device->CreateGraphicsPipelineState,
              &psoDesc,
              mPipelineState.iid(),
              mPipelineState.ppv() );

    DX12SetName( mPipelineState, "My Pipeline State" );

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

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::OffsetCpuDescHandle(
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize = m_descriptorSizes[heapType];
    const SIZE_T ptr = heapStart.ptr + iOffset * descriptorSize;
    return D3D12_CPU_DESCRIPTOR_HANDLE{ ptr };
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::OffsetGpuDescHandle(
    D3D12_GPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize = m_descriptorSizes[heapType];
    const SIZE_T ptr = heapStart.ptr + iOffset * descriptorSize;
    return D3D12_GPU_DESCRIPTOR_HANDLE{ ptr };
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::GetRTVCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_rtvHeap );
    return OffsetCpuDescHandle( m_rtvCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::GetRTVGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_rtvHeap );
    return OffsetGpuDescHandle( m_rtvGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::GetSRVCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_srvHeap );
    return OffsetCpuDescHandle( m_srvCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::GetSRVGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_srvHeap );
    return OffsetGpuDescHandle( m_srvGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i );
  }

  void DX12AppHelloTriangle::CreateRenderTargetViews( Errors& errors )
  {
    TAC_ASSERT( m_swapChain );
    TAC_ASSERT( m_device );

    // Create a RTV for each frame.
    for( UINT i = 0; i < bufferCount; i++ )
    {
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVCpuDescHandle( i );
      PCom< ID3D12Resource >& renderTarget = m_renderTargets[ i ];
      TAC_DX12_CALL( m_swapChain->GetBuffer, i, renderTarget.iid(), renderTarget.ppv() );
      m_device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      DX12SetName( renderTarget, "Render Target " + ToString( i ) );

      m_renderTargetDescs[i] = renderTarget->GetDesc();

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

                   // The associated pipeline state (IA, OM, RS, ... )
                   // that the command list will modify, all leading to a draw call?
                   ( ID3D12PipelineState* )mPipelineState );

    //m_commandList->SetPipelineState( ( ID3D12PipelineState* )mPipelineState );

    // Root signature... of the pipeline state?... which has already been created with said
    // root signature?
    //
    // You can pass nullptr to unbind the current root signature.
    //
    // Since you can share root signatures between pipelines you only need to set the root sig
    // when that should change
    m_commandList->SetGraphicsRootSignature( m_rootSignature.Get() );

    // sets the viewport of the pipeline state's rasterizer state?
    m_commandList->RSSetViewports( (UINT)m_viewports.size(), m_viewports.data() );

    // sets the scissor rect of the pipeline state's rasterizer state?
    m_commandList->RSSetScissorRects( (UINT)m_scissorRects.size(), m_scissorRects.data() );

    // Indicate that the back buffer will be used as a render target.
    TransitionRenderTarget( m_frameIndex, D3D12_RESOURCE_STATE_RENDER_TARGET );

    const Array rtCpuHDescs = { GetRTVCpuDescHandle( m_frameIndex ) };

    m_commandList->OMSetRenderTargets( ( UINT )rtCpuHDescs.size(),
                                       rtCpuHDescs.data(),
                                       false,
                                       nullptr );

    ClearRenderTargetView();


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
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVCpuDescHandle( m_frameIndex );

#if 0
    const double speed = 3;
    const auto t = ( float )Sin( ShellGetElapsedSeconds() * speed ) * 0.5f + 0.5f;

    // Record commands.
    const v4 clearColor = { t, 0.2f, 0.4f, 1.0f };
#else
    const v4 clearColor = v4{ 91, 128, 193, 255.0f } / 255.0f;
#endif
    m_commandList->ClearRenderTargetView( rtvHandle, clearColor.data(), 0, nullptr );
  }

  void DX12AppHelloTriangle::ExecuteCommandLists()
  {
    const FrameMemoryVector< ID3D12CommandList* > cmdLists =
    {
      (ID3D12CommandList*)m_commandList
    };

    // Submits an array of command lists for execution.
    m_commandQueue->ExecuteCommandLists( ( UINT )cmdLists.size(), cmdLists.data() );
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
    TAC_CALL( PreSwapChainInit, errors );
  }

  void DX12AppHelloTriangle::PreSwapChainInit( Errors& errors)
  {
    CreateDesktopWindow();
    TAC_CALL( DXGIInit, errors );
    TAC_CALL( EnableDebug, errors );
    TAC_CALL( CreateDevice, errors );
    TAC_CALL( CreateFence, errors );
    TAC_CALL( CreateCommandQueue, errors );
    TAC_CALL( CreateRTVDescriptorHeap, errors );
    //return;
    TAC_CALL( CreateInfoQueue, errors );
    TAC_CALL( CreateSRVDescriptorHeap, errors );
    TAC_CALL( CreateCommandAllocator, errors );
    TAC_CALL( CreateCommandList, errors );
    TAC_CALL( CreateRootSignature, errors );
    TAC_CALL( CreatePipelineState, errors );
  }
  void DX12AppHelloTriangle::PostSwapChainInit( Errors& errors)
  {
    if( m_swapChain )
      return;

    TAC_CALL( DX12CreateSwapChain, errors );
    TAC_CALL( CreateRenderTargetViews, errors );
    TAC_CALL( CreateVertexBuffer, errors );
    TAC_CALL( CreateSRV, errors );

    m_viewport = D3D12_VIEWPORT
    {
     .Width = ( float )m_swapChainDesc.Width,
     .Height = ( float )m_swapChainDesc.Height,
    };

    m_viewports = { m_viewport };

    m_scissorRect = D3D12_RECT
    {
      .right = ( LONG )m_swapChainDesc.Width,
      .bottom = ( LONG )m_swapChainDesc.Height,
    };

    m_scissorRects = { m_scissorRect };
  }

  void DX12AppHelloTriangle::Update( Errors& errors )
  {
    if( !GetDesktopWindowNativeHandle( hDesktopWindow ) )
      return;

    TAC_CALL( PostSwapChainInit, errors );

    // Record all the commands we need to render the scene into the command list.
    TAC_CALL( PopulateCommandList, errors );

    ExecuteCommandLists();

    TAC_CALL( SwapChainPresent, errors );

    TAC_CALL( WaitForPreviousFrame, errors );

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
