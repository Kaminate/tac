#include "tac_dx12_tutorial_2_triangle.h" // self-inc
#include "tac_dx12_tutorial_shader_compile.h"
#include "tac_dx12_tutorial_input_layout_builder.h"
#include "tac_dx12_tutorial_2_dxc.h"
#include "tac_dx12_tutorial.h"

#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_frame_vector.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-win32/tac_win32.h"

// set to true to use IASetVertexBuffers with an input layout
// set to false to use bindless through a descriptor table
// ( technically both use a vertex buffer, but only one uses input layout )
static const bool sUseInputLayout {};

#pragma comment( lib, "d3d12.lib" ) // D3D12...

const UINT myParamIndex {};

namespace Tac
{
  // -----------------------------------------------------------------------------------------------


  struct Vertex
  {
    ClipSpacePosition3 mPos;
    LinearColor3 mCol;
  };

  // -----------------------------------------------------------------------------------------------

  using namespace Render;

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Init

  void DX12AppHelloTriangle::EnableDebug( Errors& errors )
  {
    if constexpr( !kIsDebugMode )
      return;

    PCom<ID3D12Debug> dx12debug;
    TAC_DX12_CALL( D3D12GetDebugInterface( dx12debug.iid(), dx12debug.ppv() ) );

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
    if constexpr( !kIsDebugMode )
      return;

    TAC_ASSERT( m_debugLayerEnabled );

    m_device.QueryInterface( m_infoQueue );
    TAC_ASSERT( m_infoQueue );

    // Make the application debug break when bad things happen
    TAC_DX12_CALL( m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE ) );
    TAC_DX12_CALL( m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE ) );
    TAC_DX12_CALL( m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE ) );

    // First available in Windows 10 Release Preview build 20236,
    // But as of 2023-12-11 not available on my machine :(
    if( auto infoQueue1{ m_infoQueue.QueryInterface<ID3D12InfoQueue1>() } )
    {
      const D3D12MessageFunc CallbackFunc { MyD3D12MessageFunc };
      const D3D12_MESSAGE_CALLBACK_FLAGS CallbackFilterFlags { D3D12_MESSAGE_CALLBACK_FLAG_NONE };
      void* pContext { this };
      DWORD pCallbackCookie {};

      TAC_DX12_CALL( infoQueue1->RegisterMessageCallback(
                     CallbackFunc,
                     CallbackFilterFlags,
                     pContext,
                     &pCallbackCookie ) );
    }
  }

  void DX12AppHelloTriangle::CreateDevice( Errors& errors )
  {
    auto adapter { ( IDXGIAdapter* )DXGIGetBestAdapter() };
    PCom< ID3D12Device > device;
    TAC_DX12_CALL( D3D12CreateDevice(
                   adapter,
                   D3D_FEATURE_LEVEL_12_1,
                   device.iid(),
                   device.ppv() ) );
    m_device = device.QueryInterface<ID3D12Device5>();
    DX12SetName( m_device, "Device" );

    if constexpr( kIsDebugMode )
    {
      m_device.QueryInterface( m_debugDevice );
      TAC_ASSERT( m_debugDevice );
    }

    InitDescriptorSizes();
  }

  void DX12AppHelloTriangle::InitDescriptorSizes()
  {
    for( int i {}; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
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

      // This command queue manages direct command lists (direct = for graphics rendering)
      .Type { D3D12_COMMAND_LIST_TYPE_DIRECT },
    };

    TAC_DX12_CALL( m_device->CreateCommandQueue(
                   &queueDesc,
                   m_commandQueue.iid(),
                   m_commandQueue.ppv() ) );
    DX12SetName( m_commandQueue, "Command Queue" );
  }

  void DX12AppHelloTriangle::CreateRTVDescriptorHeap( Errors& errors )
  {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors
    // Descriptors are the primary unit of binding for a single resource in D3D12.

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
    // A descriptor heap is a collection of contiguous allocations of descriptors,
    // one allocation for every descriptor.
    const D3D12_DESCRIPTOR_HEAP_DESC desc
    {
      .Type { D3D12_DESCRIPTOR_HEAP_TYPE_RTV },
      .NumDescriptors { bufferCount },
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_rtvHeap.iid(),
                   m_rtvHeap.ppv() ) );
    m_rtvCpuHeapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_rtvGpuHeapStart = m_rtvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloTriangle::CreateSRVDescriptorHeap( Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc
    {
      .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV },
      .NumDescriptors { 1 },
      .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_srvHeap.iid(),
                   m_srvHeap.ppv() ) );
    m_srvCpuHeapStart = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvGpuHeapStart = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  // This is used to create a hlsl ByteAddressBuffer in DX12HelloTriangleBindless.hlsl
  // It is passed to the shader via the descriptor heap
  void DX12AppHelloTriangle::CreateSRV( Errors& errors )
  {
    TAC_ASSERT( m_vertexBuffer );

    // srv --> byteaddressbuffer
    // uav --> rwbyteaddressbuffer

    const D3D12_BUFFER_SRV Buffer
    {
      .FirstElement {},
      .NumElements  { m_vertexBufferView.SizeInBytes / 4 },
      .Flags        { D3D12_BUFFER_SRV_FLAG_RAW }, // for byteaddressbuffer
    };

    const D3D12_SHADER_RESOURCE_VIEW_DESC Desc
    {
      .Format                  { DXGI_FORMAT_R32_TYPELESS }, // for byteaddressbuffer
      .ViewDimension           { D3D12_SRV_DIMENSION_BUFFER },
      .Shader4ComponentMapping { D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING }, // swizzling?
      .Buffer                  { Buffer },
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor { GetSRVCpuDescHandle( 0 ) };
    
    m_device->CreateShaderResourceView( ( ID3D12Resource* )m_vertexBuffer,
                                        &Desc,
                                        DestDescriptor );
  }

  void DX12AppHelloTriangle::CreateCommandAllocator( Errors& errors )
  {
    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator(
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   m_commandAllocator.iid(),
                   m_commandAllocator.ppv()  ) );
    DX12SetName( m_commandAllocator, "My Command Allocator");
  }

  void DX12AppHelloTriangle::CreateCommandList( Errors& errors )
  {
    // Create the command list
    //
    // Note: CreateCommandList1 creates it the command list in a closed state, as opposed to
    //       CreateCommandList, which creates in a open state.
    PCom< ID3D12CommandList > commandList;
    TAC_DX12_CALL( m_device->CreateCommandList1(
                   0,
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   D3D12_COMMAND_LIST_FLAG_NONE,
                   commandList.iid(),
                   commandList.ppv() ) );
    TAC_ASSERT( commandList );
    commandList.QueryInterface( m_commandList );
    TAC_ASSERT( m_commandList );
    DX12SetName( m_commandList, "My Command List" );
  }

  void DX12AppHelloTriangle::CreateVertexBuffer( Errors& errors )
  {
    const float m_aspectRatio { ( float )m_swapChainDesc.Width / ( float )m_swapChainDesc.Height };

    // Define the geometry for a triangle.
    const Vertex triangleVertices[]
    {
      Vertex
      {
        .mPos { ClipSpacePosition3{0.0f, 0.25f * m_aspectRatio, 0.0f} },
        .mCol { LinearColor3{ 1.0f, 0.0f, 0.0f }}
      },
      Vertex
      {
        .mPos { ClipSpacePosition3{ -0.25f, -0.25f * m_aspectRatio, 0.0f} },
        .mCol { LinearColor3{ 0.0f, 0.0f, 1.0f }}
      },
      Vertex
      {
        .mPos { ClipSpacePosition3{ 0.25f, -0.25f * m_aspectRatio, 0.0f} },
        .mCol { LinearColor3{ 0.0f, 1.0f, 0.0f }}
      },
    };

    m_vertexBufferSize = sizeof( triangleVertices );


    const D3D12_HEAP_PROPERTIES uploadHeapProps
    {
      .Type                 { D3D12_HEAP_TYPE_UPLOAD },
      .CPUPageProperty      { D3D12_CPU_PAGE_PROPERTY_UNKNOWN },
      .MemoryPoolPreference { D3D12_MEMORY_POOL_UNKNOWN },
    };

    const DXGI_SAMPLE_DESC SampleDesc
    {
      .Count   { 1 },
      .Quality {},
    };

    const D3D12_RESOURCE_DESC resourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_BUFFER },
      .Alignment        {},
      .Width            { m_vertexBufferSize },
      .Height           { 1 },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { DXGI_FORMAT_UNKNOWN },
      .SampleDesc       { SampleDesc, },
      .Layout           { D3D12_TEXTURE_LAYOUT_ROW_MAJOR },
    };

    // must be null for buffer
    const D3D12_CLEAR_VALUE* pOptimizedClearValue {};

    // D3D12_RESOURCE_STATE_GENERIC_READ
    //   An OR'd combination of other read-state bits.
    //   The required starting state for an upload heap
    const D3D12_RESOURCE_STATES uploadHeapResourceStates { D3D12_RESOURCE_STATE_GENERIC_READ };
    TAC_CALL( m_device->CreateCommittedResource(
              &uploadHeapProps,
              D3D12_HEAP_FLAG_NONE,
              &resourceDesc,
              uploadHeapResourceStates,
              pOptimizedClearValue,
              m_vertexBufferUploadHeap.iid(),
              m_vertexBufferUploadHeap.ppv() ) );

    const D3D12_HEAP_PROPERTIES defaultHeapProps
    {
      .Type                 { D3D12_HEAP_TYPE_DEFAULT },
      .CPUPageProperty      { D3D12_CPU_PAGE_PROPERTY_UNKNOWN },
      .MemoryPoolPreference { D3D12_MEMORY_POOL_UNKNOWN },
    };

    // Creates both a resource and an implicit heap,
    // such that the heap is big enough to contain the entire resource,
    // and the resource is mapped to the heap.
    TAC_CALL( m_device->CreateCommittedResource(
      &defaultHeapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST, // we want to copy into here from the upload buffer
      pOptimizedClearValue,
      m_vertexBuffer.iid(),
      m_vertexBuffer.ppv() ) );

    DX12SetName( m_vertexBuffer, "vertexes");
    DX12SetName( m_vertexBufferUploadHeap, "vertex upload");

    // Copy the triangle data to the vertex buffer upload heap.
    const D3D12_RANGE readRange{}; // not reading from CPU
    void* pVertexDataBegin;
    TAC_DX12_CALL( m_vertexBufferUploadHeap->Map( 0, &readRange, &pVertexDataBegin ) );
    MemCpy( pVertexDataBegin, triangleVertices, m_vertexBufferSize );
    m_vertexBufferUploadHeap->Unmap( 0, nullptr );

    // Initialize the vertex buffer view.
    m_vertexBufferView = D3D12_VERTEX_BUFFER_VIEW 
    {
      .BufferLocation { m_vertexBuffer->GetGPUVirtualAddress() },
      .SizeInBytes    { m_vertexBufferSize },
      .StrideInBytes  { sizeof( Vertex ) },
    };
  }

  void DX12AppHelloTriangle::CreateFence( Errors& errors )
  {
    // Create synchronization objects.

    const UINT64 initialVal{};

    PCom< ID3D12Fence > fence;
    TAC_DX12_CALL( m_device->CreateFence(
      initialVal,
      D3D12_FENCE_FLAG_NONE,
      fence.iid(),
      fence.ppv() ) );

    fence.QueryInterface( m_fence );
    DX12SetName( fence, "fence" );

    m_fenceValue = 1;

    TAC_CALL( m_fenceEvent.Init( errors ) );
  }

  void DX12AppHelloTriangle::CreateRootSignature( Errors& errors )
  {
    // Create an empty root signature.

    const D3D12_DESCRIPTOR_RANGE1 descRange
    {
      .RangeType                         { D3D12_DESCRIPTOR_RANGE_TYPE_SRV },
      .NumDescriptors                    { 1 },
      .BaseShaderRegister                {}, // t0
      .RegisterSpace                     {}, // space0
      .Flags                             { D3D12_DESCRIPTOR_RANGE_FLAG_NONE },
      .OffsetInDescriptorsFromTableStart {},
    };

    const Array descRanges { descRange };

    const D3D12_ROOT_DESCRIPTOR_TABLE1 DescriptorTable
    {
      .NumDescriptorRanges { (UINT)descRanges.size() },
      .pDescriptorRanges   { descRanges.data() },
    };

    const D3D12_ROOT_PARAMETER1 rootParam
    {
      .ParameterType    { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE },
      .DescriptorTable  { DescriptorTable },
      .ShaderVisibility { D3D12_SHADER_VISIBILITY_VERTEX },
    };

    const Array params { rootParam };

    TAC_ASSERT( myParamIndex == 0 && params.size() > myParamIndex );

    // D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    //
    //   Omitting this flag can result in one root argument space being saved on some hardware.
    //   Omit this flag if the Input Assembler is not required, though the optimization is minor.
    //   This flat opts in to using the input assembler, which requires an input layout that
    //   defines a set of vertex buffer bindings.
    const D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = sUseInputLayout
      ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
      : D3D12_ROOT_SIGNATURE_FLAG_NONE;

    const D3D12_ROOT_SIGNATURE_DESC1 Desc_1_1
    {
      .NumParameters { (UINT)params.size() },
      .pParameters   { params.data() },
      .Flags         { rootSigFlags },
    };

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc
    {
      .Version  { D3D_ROOT_SIGNATURE_VERSION_1_1 },
      .Desc_1_1 { Desc_1_1 },
    };

    PCom<ID3DBlob> blob;

    {
      PCom<ID3DBlob> blobErr;
      const HRESULT rootSigHR{
                           D3D12SerializeVersionedRootSignature(
                           &desc,
                           blob.CreateAddress(),
                           blobErr.CreateAddress() ) };

      TAC_RAISE_ERROR_IF(
        FAILED( rootSigHR ),
        String() +
        "Failed to serialize root signature! "
        "Blob = " + ( const char* )blobErr->GetBufferPointer() + ", "
        "HRESULT = " + DX12_HRESULT_ToString( rootSigHR ) );
    }


    TAC_DX12_CALL( m_device->CreateRootSignature(
                   0,
                   blob->GetBufferPointer(),
                   blob->GetBufferSize(),
                   m_rootSignature.iid(),
                   m_rootSignature.ppv() ) );

    DX12SetName( m_rootSignature, "My Root Signature" );
  }

  void DX12AppHelloTriangle::CreatePipelineState( Errors& errors )
  {
    const AssetPathStringView shaderAssetPath{ sUseInputLayout
      ? "assets/hlsl/DX12HelloTriangle.hlsl"
      : "assets/hlsl/DX12HelloTriangleBindless.hlsl" };

    ID3D12Device* device{ m_device.Get() };

    DX12ExampleProgramCompiler::Params programCompilerParams
    {
      .mOutputDir { sShellPrefPath },
      .mDevice    { device },
    };

    TAC_CALL( DX12ExampleProgramCompiler compiler( programCompilerParams, errors ) );
    TAC_CALL( DX12ExampleProgramCompiler::Result program{ compiler.Compile( shaderAssetPath, errors ) } );

    const VertexDeclaration posDecl
    {
      .mAttribute         { Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { TAC_OFFSET_OF( Vertex, mPos ) },
    };

    const VertexDeclaration colDecl
    {
      .mAttribute         { Attribute::Color },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { TAC_OFFSET_OF( Vertex, mCol ) },
    };

    VertexDeclarations decls;
    decls.push_back( posDecl );
    decls.push_back( colDecl );

    const DX12BuiltInputLayout inputLayout{ decls };


    const D3D12_RASTERIZER_DESC RasterizerState
    {
      .FillMode              { D3D12_FILL_MODE_SOLID },
      .CullMode              { D3D12_CULL_MODE_BACK },
      .FrontCounterClockwise { true },
      .DepthBias             { D3D12_DEFAULT_DEPTH_BIAS },
      .DepthBiasClamp        { D3D12_DEFAULT_DEPTH_BIAS_CLAMP },
      .SlopeScaledDepthBias  { D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS },
      .DepthClipEnable       { true },
    };

    const D3D12_RENDER_TARGET_BLEND_DESC RTBlendDesc
    {
      // [x] Q: Why is BlendEnable = false? Why not just leave it out?
      //     A: You can leave it out.
#if 0
      .BlendEnable           {},
      .LogicOpEnable         {},
      .SrcBlend              { D3D12_BLEND_ONE },
      .DestBlend             { D3D12_BLEND_ZERO },
      .BlendOp               { D3D12_BLEND_OP_ADD },
      .SrcBlendAlpha         { D3D12_BLEND_ONE },
      .DestBlendAlpha        { D3D12_BLEND_ZERO },
      .BlendOpAlpha          { D3D12_BLEND_OP_ADD },
      .LogicOp               { D3D12_LOGIC_OP_NOOP },
#endif
      .RenderTargetWriteMask { D3D12_COLOR_WRITE_ENABLE_ALL },
    };

    const D3D12_BLEND_DESC BlendState { .RenderTarget { RTBlendDesc }, };


    const D3D12_INPUT_LAYOUT_DESC InputLayout{ sUseInputLayout
          ? ( D3D12_INPUT_LAYOUT_DESC )inputLayout
          : D3D12_INPUT_LAYOUT_DESC{} };

    const DXGI_FORMAT rtvDXVIFmt{ TexFmtToDxgiFormat( mRTVFmt ) };

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      .pRootSignature        { ( ID3D12RootSignature* )m_rootSignature },
      .VS                    { program.mVSBytecode },
      .PS                    { program.mPSBytecode },
      .BlendState            { BlendState },
      .SampleMask            { UINT_MAX },
      .RasterizerState       { RasterizerState },
      .DepthStencilState     { D3D12_DEPTH_STENCIL_DESC{} },
      .InputLayout           { InputLayout },
      .PrimitiveTopologyType { D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
      .NumRenderTargets      { 1 },
      .RTVFormats            { rtvDXVIFmt },
      .SampleDesc            { .Count { 1 } },
    };
    TAC_CALL( m_device->CreateGraphicsPipelineState(
              &psoDesc,
              mPipelineState.iid(),
              mPipelineState.ppv() ));

    DX12SetName( mPipelineState, "My Pipeline State" );

  }

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Update

  void DX12AppHelloTriangle::DX12CreateSwapChain( HWND hwnd, v2i size, Errors& errors )
  {
    TAC_ASSERT( m_commandQueue );
    TAC_ASSERT( hwnd );

    const DXGISwapChainWrapper::Params scInfo
    {
      .mHwnd        { hwnd },
      .mDevice      { ( IUnknown* )m_commandQueue }, // swap chain can force flush the queue
      .mBufferCount { bufferCount },
      .mWidth       { size.x },
      .mHeight      { size.y },
      .mFmt         { TexFmtToDxgiFormat( mRTVFmt ) },
    };
    TAC_CALL( m_swapChain.Init( scInfo, errors ));
    TAC_CALL( m_swapChain->GetDesc1( &m_swapChainDesc ) );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::OffsetCpuDescHandle(
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize { m_descriptorSizes[heapType] };
    const SIZE_T ptr { heapStart.ptr + iOffset * descriptorSize };
    return D3D12_CPU_DESCRIPTOR_HANDLE{ ptr };
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloTriangle::OffsetGpuDescHandle(
    D3D12_GPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize { m_descriptorSizes[heapType] };
    const SIZE_T ptr { heapStart.ptr + iOffset * descriptorSize };
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
    for( UINT i {}; i < bufferCount; i++ )
    {
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle { GetRTVCpuDescHandle( i ) };
      PCom< ID3D12Resource >& renderTarget { m_renderTargets[ i ] };
      TAC_DX12_CALL( m_swapChain->GetBuffer( i, renderTarget.iid(), renderTarget.ppv() ) );
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

    const D3D12_RESOURCE_TRANSITION_BARRIER Transition
    {
      .pResource   { rtResource },
      .Subresource { D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES },
      .StateBefore { before },
      .StateAfter  { targetState },
    };

    const D3D12_RESOURCE_BARRIER barrier
    {
      .Type       { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
      .Transition { Transition      },
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
    const Array barriers  { barrier };
    const UINT rtN { ( UINT )barriers.size() };
    const D3D12_RESOURCE_BARRIER* rts { barriers.data() };
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
    TAC_DX12_CALL( m_commandAllocator->Reset() );

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    //
    // ID3D12GraphicsCommandList::Reset
    //
    //   Resets a command list back to its initial state as if a new command list was just created.
    //   After Reset succeeds, the command list is left in the "recording" state.
    //
    //   you can re-use command list tracking structures without any allocations
    //   you can call Reset while the command list is still being executed
    //   you can submit a cmd list, reset it, and reuse the allocated memory for another cmd list
    TAC_DX12_CALL( m_commandList->Reset(
                   ( ID3D12CommandAllocator* )m_commandAllocator,

                   // The associated pipeline state (IA, OM, RS, ... )
                   // that the command list will modify, all leading to a draw call?
                   ( ID3D12PipelineState* )mPipelineState ) );

    if( !m_vertexBufferCopied )
    {
      m_vertexBufferCopied = true;
      const D3D12_RESOURCE_STATES StateAfter{ sUseInputLayout
        // vtx buffer (input layout) or constant buffer
          ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER

        // byteaddressbuffer, used in vertex (non-pixel) shader
          : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE };

      const D3D12_RESOURCE_TRANSITION_BARRIER Transition
      {
        .pResource   { (ID3D12Resource*)m_vertexBuffer },
        .StateBefore { D3D12_RESOURCE_STATE_COPY_DEST },
        .StateAfter  {  StateAfter },
      };

      const D3D12_RESOURCE_BARRIER barrier
      {
        .Type       { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
        .Transition { Transition },
      };
      const Array barriers{ barrier };

      m_commandList->CopyBufferRegion( ( ID3D12Resource* )m_vertexBuffer,
                                       0,
                                       ( ID3D12Resource* )m_vertexBufferUploadHeap,
                                       0,
                                       m_vertexBufferSize );

      m_commandList->ResourceBarrier( ( UINT )barriers.size(), barriers.data() );
    }

    // no need to call ID3D12GraphicsCommandList::SetPipelineState( ID3D12PipelineState* ), I think
    // that's implicitly done by ID3D12GraphicsCommandList::Reset( ..., ID3D12PipelineState* )

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

    const Array rtCpuHDescs{ GetRTVCpuDescHandle( m_frameIndex ) };

    m_commandList->OMSetRenderTargets( ( UINT )rtCpuHDescs.size(),
                                       rtCpuHDescs.data(),
                                       false,
                                       nullptr );

    ClearRenderTargetView();

    if( sUseInputLayout )
    {
      const Array vbViews { m_vertexBufferView };
      m_commandList->IASetVertexBuffers(0, (UINT)vbViews.size(), vbViews.data() );
    }
    else
    {
      // ...
      const Array descHeaps { ( ID3D12DescriptorHeap* )m_srvHeap };
      m_commandList->SetDescriptorHeaps( ( UINT )descHeaps.size(), descHeaps.data() );

      // ...
      const UINT RootParameterIndex {};
      static_assert( RootParameterIndex == myParamIndex );
      m_commandList->SetGraphicsRootDescriptorTable( RootParameterIndex, m_srvGpuHeapStart );
    }

    m_commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    const D3D12_DRAW_ARGUMENTS drawArgs
    {
      .VertexCountPerInstance { 3 },
      .InstanceCount          { 1 },
      .StartVertexLocation    {},
      .StartInstanceLocation  {},
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
    TAC_DX12_CALL( m_commandList->Close() );
  }

  void DX12AppHelloTriangle::ClearRenderTargetView()
  {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle { GetRTVCpuDescHandle( m_frameIndex ) };
    const v4 clearColor { v4{ mT * 255, 128, 193, 255.0f } / 255.0f };
    m_commandList->ClearRenderTargetView( rtvHandle, clearColor.data(), 0, nullptr );
  }

  void DX12AppHelloTriangle::ExecuteCommandLists()
  {
    const FrameMemoryVector< ID3D12CommandList* > cmdLists
    {
      ( ID3D12CommandList* )m_commandList,
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


    TAC_CALL( CheckSwapEffect( m_swapChainDesc.SwapEffect, errors ) );

    const DXGI_PRESENT_PARAMETERS params{};

    // For the flip model (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL), values are:
    //   0   - Cancel the remaining time on the previously presented frame
    //         and discard this frame if a newer frame is queued.
    //   1-4 - Synchronize presentation for at least n vertical blanks.
    const UINT SyncInterval { 1 };
    const UINT PresentFlags {};

    // I think this technically adds a frame onto the present queue
    TAC_DX12_CALL( m_swapChain->Present1( SyncInterval, PresentFlags, &params ) );
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

    const UINT64 signalValue { m_fenceValue };

    // Use this method to set a fence value from the GPU side
    // [ ] Q: ^ ???
    TAC_DX12_CALL( m_commandQueue->Signal( (ID3D12Fence*)m_fence, signalValue ) );

    m_fenceValue++;

    // Experimentally, m_fence->GetCompletedValue() doesn't hit the signalled value until

    // ID3D12Fence::GetCompletedValue
    // - Gets the current value of the fence.
    //
    // Wait until the previous frame is finished.

    // I think this if statement is used because the alternative
    // would be while( m_fence->GetCompletedValue() != fence ) { TAC_NO_OP; }
    const UINT64 curValue { m_fence->GetCompletedValue() };
    if( curValue < signalValue )
    {
      // m_fenceEvent is only ever used in this scope 

      // ID3D12Fence::SetEventOnCompletion
      // - Specifies an event that's raised when the fence reaches a certain value.
      //
      // the event will be 'complete' when it reaches the specified value.
      // This value is set by the cmdqueue::Signal
      TAC_DX12_CALL( m_fence->SetEventOnCompletion( signalValue, (HANDLE)m_fenceEvent ) );
      WaitForSingleObject( (HANDLE)m_fenceEvent, INFINITE );
    }

    // I think this would be the same if it were called after IDXGISwapChain::Present instead
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  }


  // -----------------------------------------------------------------------------------------------

  DX12AppHelloTriangle::DX12AppHelloTriangle( const Config& cfg ) : App( cfg ) {}

  void DX12AppHelloTriangle::Init( Errors& errors )
  {
    AppWindowApi::SetSwapChainAutoCreate( false );

    TAC_CALL( hDesktopWindow = DX12ExampleCreateWindow("DX12 Window", errors ) );

    TAC_CALL( DXGIInit( errors ) );
    TAC_CALL( EnableDebug( errors ) );
    TAC_CALL( CreateDevice( errors ) );
    TAC_CALL( CreateFence( errors ) );
    TAC_CALL( CreateCommandQueue( errors ) );
    TAC_CALL( CreateRTVDescriptorHeap( errors ) );
    TAC_CALL( CreateInfoQueue( errors ) );
    TAC_CALL( CreateSRVDescriptorHeap( errors ) );
    TAC_CALL( CreateCommandAllocator( errors ) );
    TAC_CALL( CreateCommandList( errors ) );
    TAC_CALL( CreateRootSignature( errors ) );
    TAC_CALL( CreatePipelineState( errors ) );
  }

  void DX12AppHelloTriangle::PostSwapChainInit( Errors& errors)
  {
    TAC_CALL( CreateRenderTargetViews( errors ) );
    TAC_CALL( CreateVertexBuffer( errors ) );
    TAC_CALL( CreateSRV( errors ) );

    m_viewport = D3D12_VIEWPORT
    {
     .Width   { ( float )m_swapChainDesc.Width },
     .Height  { ( float )m_swapChainDesc.Height },
    };

    m_scissorRect = D3D12_RECT
    {
      .right  { ( LONG )m_swapChainDesc.Width },
      .bottom { ( LONG )m_swapChainDesc.Height },
    };

    m_viewports = { m_viewport };
    m_scissorRects = { m_scissorRect };
  }

  void DX12AppHelloTriangle::Render( RenderParams renderParams, Errors& errors )
  {
    const auto hwnd{ ( HWND )AppWindowApi::GetNativeWindowHandle( hDesktopWindow ) };
    if( !hwnd )
      return;

    if( !m_swapChain )
    {
      const v2i size{ AppWindowApi::GetSize( hDesktopWindow ) };
      TAC_CALL( DX12CreateSwapChain( hwnd, size, errors ) );
      TAC_CALL( PostSwapChainInit( errors ) );
    }
    
    if( !AppWindowApi::IsShown( hDesktopWindow ) )
      return;
    const double t { Lerp( renderParams.mOldState->mTimestamp.mSeconds, 
                           renderParams.mNewState->mTimestamp.mSeconds, 
                           renderParams.mT ) };
    mT = ( float )Sin( t ) * .5f + .5f;

    // Record all the commands we need to render the scene into the command list.
    TAC_CALL( PopulateCommandList( errors ) );

    ExecuteCommandLists();

    TAC_CALL( SwapChainPresent( errors ) );

    TAC_CALL( WaitForPreviousFrame( errors ) );
  }

  void DX12AppHelloTriangle::Update( Errors& errors )
  {
  }

  void DX12AppHelloTriangle::Uninit( Errors& errors )
  {
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    TAC_CALL( WaitForPreviousFrame( errors ));

    DXGIUninit();
  }

  App* App::Create()
  {
    const App::Config config
    {
      .mName            { "DX12 Hello Triangle" },
      .mDisableRenderer { true },
    };
    return TAC_NEW DX12AppHelloTriangle( config );
  };


} // namespace Tac

