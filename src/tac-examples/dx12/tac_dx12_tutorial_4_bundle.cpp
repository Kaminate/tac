#include "tac_dx12_tutorial_4_bundle.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-win32/tac_win32.h"
#include "tac_dx12_tutorial.h"
#include "tac_dx12_tutorial_2_dxc.h"
#include "tac_dx12_tutorial_checkerboard.h"
#include "tac_dx12_tutorial_input_layout_builder.h"
#include "tac_dx12_tutorial_root_sig_builder.h"
#include "tac_dx12_tutorial_shader_compile.h"

#pragma comment( lib, "d3d12.lib" ) // D3D12...

static const UINT myParamIndex { 0 };


namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  struct Vertex
  {
    ClipSpacePosition3 mPos;
    LinearColor3       mCol;
    TextureCoordinate2 mUVs;
  };

  // -----------------------------------------------------------------------------------------------

  using namespace Render;


  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Init


  void DX12AppHelloBundle::EnableDebug( Errors& errors )
  {
    if constexpr( !kIsDebugMode )
      return;

    PCom<ID3D12Debug> dx12debug;
    TAC_DX12_CALL( D3D12GetDebugInterface( dx12debug.iid(), dx12debug.ppv() ) );

    dx12debug.QueryInterface( m_debug );

    // EnableDebugLayer must be called before the device is created
    TAC_ASSERT( !m_device );
    m_debug->EnableDebugLayer();

    // ( this should already be enabled by default )
    m_debug->SetEnableSynchronizedCommandQueueValidation( TRUE );

    // https://learn.microsoft.com
    // GPU-based validation can be enabled only prior to creating a device. Disabled by default.
    //
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation
    // GPU-based validation helps to identify the following errors:
    // - Use of uninitialized or incompatible descriptors in a shader.
    // - Use of descriptors referencing deleted Resources in a shader.
    // - Validation of promoted resource states and resource state decay.
    // - Indexing beyond the end of the descriptor heap in a shader.
    // - Shader accesses of resources in incompatible state.
    // - Use of uninitialized or incompatible Samplers in a shader.
    m_debug->SetEnableGPUBasedValidation( TRUE );

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

  void DX12AppHelloBundle::CreateInfoQueue( Errors& errors )
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
    if( auto infoQueue1 = m_infoQueue.QueryInterface<ID3D12InfoQueue1>() )
    {
      const D3D12MessageFunc CallbackFunc = MyD3D12MessageFunc;
      const D3D12_MESSAGE_CALLBACK_FLAGS CallbackFilterFlags = D3D12_MESSAGE_CALLBACK_FLAG_NONE;
      void* pContext = this;
      DWORD pCallbackCookie = 0;

      TAC_DX12_CALL( infoQueue1->RegisterMessageCallback(
                     CallbackFunc,
                     CallbackFilterFlags,
                     pContext,
                     &pCallbackCookie ) );
    }
  }

  void DX12AppHelloBundle::CreateDevice( Errors& errors )
  {
    auto adapter = ( IDXGIAdapter* )DXGIGetBestAdapter();
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

  void DX12AppHelloBundle::InitDescriptorSizes()
  {
    for( int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
      m_descriptorSizes[ i ]
      = m_device->GetDescriptorHandleIncrementSize( ( D3D12_DESCRIPTOR_HEAP_TYPE )i );
  }

  void DX12AppHelloBundle::CreateCommandQueue( Errors& errors )
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
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    TAC_DX12_CALL( m_device->CreateCommandQueue(
                   &queueDesc,
                   m_commandQueue.iid(),
                   m_commandQueue.ppv() ) );
    DX12SetName( m_commandQueue, "Command Queue" );
  }

  void DX12AppHelloBundle::CreateRTVDescriptorHeap( Errors& errors )
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
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_rtvHeap.iid(),
                   m_rtvHeap.ppv() ) );
    m_rtvCpuHeapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_rtvGpuHeapStart = m_rtvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloBundle::CreateSamplerDescriptorHeap( Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = ( UINT )1,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_samplerHeap.iid(),
                   m_samplerHeap.ppv() ) );
    m_samplerCpuHeapStart = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
    m_samplerGpuHeapStart = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
  }


  void DX12AppHelloBundle::CreateSRVDescriptorHeap( Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = ( UINT )SRVIndexes::Count,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_srvHeap.iid(),
                   m_srvHeap.ppv() ) );
    m_srvCpuHeapStart = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvGpuHeapStart = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloBundle::CreateVertexBufferSRV( Errors& errors )
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
        .NumElements = m_vertexBufferByteCount / 4,
        .Flags = D3D12_BUFFER_SRV_FLAG_RAW, // for byteaddressbuffer
      },
    };


    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor =
      GetSRVCpuDescHandle( SRVIndexes::TriangleVertexBuffer );
    
    m_device->CreateShaderResourceView( ( ID3D12Resource* )m_vertexBuffer,
                                        &Desc,
                                        DestDescriptor );
  }

  void DX12AppHelloBundle::CreateSampler( Errors& errors )
  {
    const D3D12_SAMPLER_DESC Desc
    {
      .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
      .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
      .MinLOD = 0,
      .MaxLOD = D3D12_FLOAT32_MAX,
    };
    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = GetSamplerCpuDescHandle( 0 );
    m_device->CreateSampler( &Desc, DestDescriptor );
  }


  void DX12AppHelloBundle::CreateTexture( Errors& errors )
  {
    
    static bool sCreated;
    if( sCreated )
      return;
    sCreated = true;

    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    PCom<ID3D12Resource> textureUploadHeap;


    // Create the texture.

    const DXGI_SAMPLE_DESC SampleDesc{ .Count { 1 }, .Quality {} };

    // Describe and create a Texture2D.
    const D3D12_RESOURCE_DESC resourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_TEXTURE2D },
      .Width            { Checkerboard::TextureWidth },
      .Height           { Checkerboard::TextureHeight },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { DXGI_FORMAT_R8G8B8A8_UNORM },
      .SampleDesc       { SampleDesc },
    };

    m_textureDesc = resourceDesc;

    const D3D12_HEAP_PROPERTIES defaultHeapProps { .Type = D3D12_HEAP_TYPE_DEFAULT, };

    TAC_CALL( m_device->CreateCommittedResource(
      &defaultHeapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      m_texture.iid(),
      m_texture.ppv() ) );

    m_textureResourceStates = D3D12_RESOURCE_STATE_COPY_DEST;

    UINT64 totalBytes;
    m_device->GetCopyableFootprints(
      &resourceDesc, 0, 1, 0, nullptr, nullptr, nullptr, &totalBytes );

    const D3D12_HEAP_PROPERTIES uploadHeapProps { .Type = D3D12_HEAP_TYPE_UPLOAD, };

    const D3D12_RESOURCE_DESC uploadBufferResourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_BUFFER },
      .Width            { totalBytes },
      .Height           { 1 },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .SampleDesc       { DXGI_SAMPLE_DESC {.Count { 1 }, .Quality {}, } },
      .Layout           { D3D12_TEXTURE_LAYOUT_ROW_MAJOR },
    };

    // Create the GPU upload buffer.
    TAC_CALL( m_device->CreateCommittedResource(
      &uploadHeapProps,
      D3D12_HEAP_FLAG_NONE,
      &uploadBufferResourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      textureUploadHeap.iid(),
      textureUploadHeap.ppv() ) );

    // Copy data to the intermediate upload heap and then schedule a copy 
    // from the upload heap to the Texture2D.
    const Vector<UINT8> texture = Checkerboard::GenerateCheckerboardTextureData();

    const D3D12_SUBRESOURCE_DATA textureData =
    {
      .pData = texture.data(),
      .RowPitch = Checkerboard::TexturePixelSize * Checkerboard::TextureWidth,
      .SlicePitch = Checkerboard::TexturePixelSize * Checkerboard::TextureWidth * Checkerboard::TextureHeight,
    };

    // --- update subresource begin ---

    const int nSubRes { 1 };
    Vector< D3D12_PLACED_SUBRESOURCE_FOOTPRINT > footprints( nSubRes ); // aka layouts?
    Vector< UINT64 > rowByteCounts( nSubRes );
    Vector< UINT > rowCounts( nSubRes );
    UINT64 requiredByteCount;
    m_device->GetCopyableFootprints( &m_textureDesc,
                                     0, // first subresource
                                     nSubRes,
                                     0, // base offset
                                     footprints.data(),
                                     rowCounts.data(),
                                     rowByteCounts.data(),
                                     &requiredByteCount );

    // for each subresource
    for( int subresourceIndex {}; subresourceIndex < nSubRes; ++subresourceIndex )
    {

      TAC_ASSERT( totalBytes >= requiredByteCount );

      const D3D12_RANGE readRange{}; // not reading from CPU
      void* mappedData;
      TAC_DX12_CALL( textureUploadHeap->Map( (UINT)subresourceIndex, &readRange, &mappedData ) );

      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = footprints[ subresourceIndex ];
      const UINT rowCount = rowCounts[ subresourceIndex];

      const D3D12_MEMCPY_DEST DestData 
      {
        .pData { (char*)mappedData + layout.Offset },
        .RowPitch { layout.Footprint.RowPitch },
        .SlicePitch { SIZE_T( layout.Footprint.RowPitch ) * SIZE_T( rowCount ) },
      };

      const int rowByteCount { ( int )rowByteCounts[ subresourceIndex ] };

      const UINT NumSlices { layout.Footprint.Depth };

      // For each slice
      for (UINT z {}; z < NumSlices; ++z)
      {
          auto pDestSlice { (BYTE*)DestData.pData + DestData.SlicePitch * z };
          auto pSrcSlice { (const BYTE*)textureData.pData + textureData.SlicePitch * LONG_PTR(z) };
          for (UINT y {}; y < rowCount; ++y)
          {
            void* dst { pDestSlice + DestData.RowPitch * y };
            const void* src { pSrcSlice + textureData.RowPitch * LONG_PTR(y) };

            MemCpy(dst, src, rowByteCount);
          }
      }


      textureUploadHeap->Unmap( subresourceIndex, nullptr);
    }

    TAC_DX12_CALL( m_commandAllocator->Reset() );
    TAC_DX12_CALL( m_commandList->Reset(
      ( ID3D12CommandAllocator* )m_commandAllocator,
      nullptr ) );


    for( int iSubRes {}; iSubRes < nSubRes; ++iSubRes )
    {
        const D3D12_TEXTURE_COPY_LOCATION Dst
        {
          .pResource        { (ID3D12Resource *)m_texture },
          .Type             { D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX },
          .SubresourceIndex { (UINT)iSubRes },
        };

        const D3D12_TEXTURE_COPY_LOCATION Src
        {
          .pResource       { (ID3D12Resource*)textureUploadHeap },
          .Type            { D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT },
          .PlacedFootprint { footprints[ iSubRes ] },
        };

        m_commandList->CopyTextureRegion( &Dst, 0, 0, 0, &Src, nullptr );
    }

    // --- update subresource end ---

    const TransitionParams transitionParams
    {
       .mResource { ( ID3D12Resource* )m_texture },
       .mCurrentState { &m_textureResourceStates },
       .mTargetState { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
    };

    TransitionResource( transitionParams );

    // Describe and create a SRV for the texture.
    const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc 
    {
      .Format                  { resourceDesc.Format },
      .ViewDimension           { D3D12_SRV_DIMENSION_TEXTURE2D },
      .Shader4ComponentMapping { D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING },
      .Texture2D               { D3D12_TEX2D_SRV {.MipLevels { 1 }, } },
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor { GetSRVCpuDescHandle( SRVIndexes::TriangleTexture ) };
    m_device->CreateShaderResourceView((ID3D12Resource*)m_texture.Get(),
                                        &srvDesc,
                                        DestDescriptor);
    
    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( m_commandList->Close() );

    ExecuteCommandLists();

    // wait until assets have been uploaded to the GPU.
    // Wait for the command list to execute; we are reusing the same command 
    // list in our main loop but for now, we just want to wait for setup to 
    // complete before continuing.
    TAC_CALL( WaitForPreviousFrame( errors ) );
  }


  void DX12AppHelloBundle::TransitionResource( TransitionParams params )
  {
    const D3D12_RESOURCE_STATES StateBefore { *params.mCurrentState };

    TAC_ASSERT( params.mResource );
    TAC_ASSERT( StateBefore != params.mTargetState );

    const D3D12_RESOURCE_TRANSITION_BARRIER Transition 
    {
      .pResource   { params.mResource },
      .Subresource { D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES },
      .StateBefore { StateBefore },
      .StateAfter  { params.mTargetState },
    };

    const D3D12_RESOURCE_BARRIER barrier
    {
      .Type { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
      .Transition { Transition },
    };

    ResourceBarrier( barrier );

    *params.mCurrentState = params.mTargetState;
  }

  void DX12AppHelloBundle::CreateCommandAllocator( Errors& errors )
  {
    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator(
                   D3D12_COMMAND_LIST_TYPE_DIRECT,
                   m_commandAllocator.iid(),
                   m_commandAllocator.ppv()  ) );
    DX12SetName( m_commandAllocator, "My Command Allocator");
  }

  void DX12AppHelloBundle::CreateCommandAllocatorBundle( Errors& errors )
  {
    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator(
                   D3D12_COMMAND_LIST_TYPE_BUNDLE,
                   m_commandAllocatorBundle.iid(),
                   m_commandAllocatorBundle.ppv()  ) );
    DX12SetName( m_commandAllocatorBundle, "Bundle Allocator");
  }

  void DX12AppHelloBundle::CreateCommandList( Errors& errors )
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

  void DX12AppHelloBundle::CreateCommandListBundle( Errors& errors )
  {
    // Create the command list
    //
    // Note: CreateCommandList1 creates it the command list in a closed state, as opposed to
    //       CreateCommandList, which creates in a open state.
    PCom< ID3D12CommandList > commandList;
    TAC_DX12_CALL( m_device->CreateCommandList1(
                   0,
                   D3D12_COMMAND_LIST_TYPE_BUNDLE,
                   D3D12_COMMAND_LIST_FLAG_NONE,
                   commandList.iid(),
                   commandList.ppv() ) );

    TAC_ASSERT( commandList );
    commandList.QueryInterface( m_commandListBundle );
    TAC_ASSERT( m_commandListBundle );
    DX12SetName( m_commandListBundle, "my_bundle_cmdlist" );
  }

  void DX12AppHelloBundle::CreateVertexBuffer( Errors& errors )
  {
    static bool sCreated;
    if( sCreated )
      return;
    sCreated = true;

    const float m_aspectRatio { ( float )m_swapChainDesc.Width / ( float )m_swapChainDesc.Height };

    // Define the geometry for a triangle.
    const Vertex triangleVertices[]
    {
      Vertex
      {
        .mPos { ClipSpacePosition3{0.0f, 0.25f * m_aspectRatio, 0.0f} },
        .mCol { LinearColor3{ 1.0f, 0.0f, 0.0f} },
        .mUVs { TextureCoordinate2{.5f, 0} },
      },
      Vertex
      {
        .mPos { ClipSpacePosition3{ -0.25f, -0.25f * m_aspectRatio, 0.0f} },
        .mCol { LinearColor3{ 0.0f, 0.0f, 1.0f} },
        .mUVs { TextureCoordinate2{0,1} },
      },
      Vertex
      {
        .mPos { ClipSpacePosition3{ 0.25f, -0.25f * m_aspectRatio, 0.0f} },
        .mCol { LinearColor3{ 0.0f, 1.0f, 0.0f} },
        .mUVs { TextureCoordinate2{1,1} },
      },
    };

    m_vertexBufferByteCount = sizeof( triangleVertices );

    const D3D12_HEAP_PROPERTIES uploadHeapProps{ .Type { D3D12_HEAP_TYPE_UPLOAD }, };

    const D3D12_RESOURCE_DESC resourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_BUFFER },
      .Alignment        {},
      .Width            { m_vertexBufferByteCount },
      .Height           { 1 },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { DXGI_FORMAT_UNKNOWN },
      .SampleDesc       { .Count { 1 }, .Quality {}, },
      .Layout           { D3D12_TEXTURE_LAYOUT_ROW_MAJOR },
    };

    // must be null for resources of dimension buffer
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

    const D3D12_HEAP_PROPERTIES defaultHeapProps{ .Type { D3D12_HEAP_TYPE_DEFAULT }, };

    // we want to copy into here from the upload buffer
    D3D12_RESOURCE_STATES defaultHeapState { D3D12_RESOURCE_STATE_COPY_DEST };

    // Creates both a resource and an implicit heap,
    // such that the heap is big enough to contain the entire resource,
    // and the resource is mapped to the heap.
    TAC_CALL( m_device->CreateCommittedResource(
      &defaultHeapProps,
      D3D12_HEAP_FLAG_NONE,
      &resourceDesc,
      defaultHeapState, 
      pOptimizedClearValue,
      m_vertexBuffer.iid(),
      m_vertexBuffer.ppv() ) );

    DX12SetName( m_vertexBuffer, "vtxbuf");
    DX12SetName( m_vertexBufferUploadHeap, "vtx upload buf");

    // Copy the triangle data to the vertex buffer upload heap.
    {
      // An empty range indicates that we are not reading from CPU.
      // A nullptr indicates that the entire subresource may be read by the CPU
      const D3D12_RANGE readRange{};

      void* pVertexDataBegin;
      TAC_DX12_CALL( m_vertexBufferUploadHeap->Map( 0, &readRange, &pVertexDataBegin ) );
      MemCpy( pVertexDataBegin, triangleVertices, m_vertexBufferByteCount );
      m_vertexBufferUploadHeap->Unmap( 0, nullptr );
    }

    if( !m_vertexBufferCopied )
    {
      m_vertexBufferCopied = true;


      TAC_DX12_CALL( m_commandAllocator->Reset() );
      TAC_DX12_CALL( m_commandList->Reset(
        ( ID3D12CommandAllocator* )m_commandAllocator,
        nullptr ) );

      m_commandList->CopyBufferRegion( ( ID3D12Resource* )m_vertexBuffer, // dst
                                       0, // dstoffest
                                       ( ID3D12Resource* )m_vertexBufferUploadHeap, // src
                                       0, // srcoffset
                                       m_vertexBufferByteCount );

      const TransitionParams transitionParams
      {
        .mResource     { (ID3D12Resource*)m_vertexBuffer },
        .mCurrentState { &defaultHeapState },

        // for the vertex shader byteaddressbuffer
        .mTargetState  { D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE },
      };
      TransitionResource( transitionParams );
    }

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( m_commandList->Close() );

    ExecuteCommandLists();

    // wait until assets have been uploaded to the GPU.
    // Wait for the command list to execute; we are reusing the same command 
    // list in our main loop but for now, we just want to wait for setup to 
    // complete before continuing.
    TAC_CALL( WaitForPreviousFrame( errors ) );

    TAC_CALL( CreateVertexBufferSRV( errors ) );
  }

  void DX12AppHelloBundle::CreateFence( Errors& errors )
  {
    // Create synchronization objects.

    const UINT64 initialVal {};

    PCom< ID3D12Fence > fence;
    TAC_DX12_CALL( m_device->CreateFence(
                   initialVal,
                   D3D12_FENCE_FLAG_NONE,
                   fence.iid(),
                   fence.ppv() ) );

    fence.QueryInterface(m_fence);
    DX12SetName( fence, "fence" );

    m_fenceValue = 1;

    TAC_CALL( m_fenceEvent.Init( errors ) );
  }



  void DX12AppHelloBundle::CreateRootSignature( Errors& errors )
  {
    DX12ExampleRootSignatureBuilder builder( ( ID3D12Device* )m_device );

    // register(s0)  samplers
    builder.AddRootDescriptorTable( D3D12_SHADER_VISIBILITY_PIXEL,
                                    D3D12_DESCRIPTOR_RANGE1{
                                      .RangeType      { D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER },
                                      .NumDescriptors { 1 },
                                    } );

    // register(t0+, space0) vertex buffers
    builder.AddRootDescriptorTable( D3D12_SHADER_VISIBILITY_VERTEX,
                                    D3D12_DESCRIPTOR_RANGE1{
                                      .RangeType          { D3D12_DESCRIPTOR_RANGE_TYPE_SRV },
                                      .NumDescriptors     { 1 },
                                      .BaseShaderRegister {},
                                      .RegisterSpace      {},
                                    } );

    // register(t0+, space1) textures
    builder.AddRootDescriptorTable( D3D12_SHADER_VISIBILITY_PIXEL,
                                    D3D12_DESCRIPTOR_RANGE1{
                                      .RangeType          { D3D12_DESCRIPTOR_RANGE_TYPE_SRV },
                                      .NumDescriptors     { 1 },
                                      .BaseShaderRegister {},
                                      .RegisterSpace      { 1 },
                                    } );

    m_rootSignature = TAC_CALL( builder.Build( errors ) );
    DX12SetName( m_rootSignature, "my_root_signature" );
  }




  void DX12AppHelloBundle::CreatePipelineState( Errors& errors )
  {
    const AssetPathStringView shaderAssetPath { "assets/hlsl/DX12HelloBundle.hlsl" };

    const DX12ExampleProgramCompiler::Params programInput
    {
      .mOutputDir { sShellPrefPath },
      .mDevice    { ( ID3D12Device* )m_device },
    };
    TAC_CALL( DX12ExampleProgramCompiler compiler( programInput, errors ) );

    TAC_CALL( DX12ExampleProgramCompiler::Result compileResult{ compiler.Compile( shaderAssetPath, errors ) } );

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

    const D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc
    {
      .RenderTargetWriteMask { D3D12_COLOR_WRITE_ENABLE_ALL },
    };

    const D3D12_BLEND_DESC BlendState
    {
      .RenderTarget { RenderTargetBlendDesc },
    };

    const DXGI_SAMPLE_DESC SampleDesc{ .Count{ 1 } };

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
      // If the root signature doesn't match shader state, it throws an error
      .pRootSignature        { ( ID3D12RootSignature* )m_rootSignature },
      .VS                    { compileResult.mVSBytecode },
      .PS                    { compileResult.mPSBytecode },
      .BlendState            { BlendState },
      .SampleMask            { UINT_MAX },
      .RasterizerState       { RasterizerState },
      .DepthStencilState     { D3D12_DEPTH_STENCIL_DESC{} },
      .InputLayout           { D3D12_INPUT_LAYOUT_DESC{} },
      .PrimitiveTopologyType { D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE },
      .NumRenderTargets      { 1 },
      .RTVFormats            { RTVFormat },
      .SampleDesc            { SampleDesc },
    };
    TAC_CALL( m_device->CreateGraphicsPipelineState(
              &psoDesc,
              mPipelineState.iid(),
              mPipelineState.ppv() ) );

    DX12SetName( mPipelineState, "My Pipeline State" );

  }

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Update

  void DX12AppHelloBundle::DX12CreateSwapChain( Errors& errors )
  {
    if( m_swapChainValid )
      return;

    auto hwnd { ( HWND )AppWindowApi::GetNativeWindowHandle( hDesktopWindow ) };
    if( !hwnd )
      return;

    const v2i size { AppWindowApi::GetSize( hDesktopWindow ) };


    TAC_ASSERT( m_commandQueue );

    const DXGISwapChainWrapper::Params scInfo
    {
      .mHwnd        { hwnd },
      .mDevice      { (IUnknown*)m_commandQueue }, // swap chain can force flush the queue
      .mBufferCount { bufferCount },
      .mWidth       { size.x },
      .mHeight      { size.y },
      .mFmt         { RTVFormat },
    };
    TAC_CALL( m_swapChain.Init( scInfo, errors ) );
    TAC_CALL( m_swapChain->GetDesc1( &m_swapChainDesc ) );
    m_swapChainValid = true;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::OffsetCpuDescHandle(
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize { m_descriptorSizes[heapType] };
    const SIZE_T ptr { heapStart.ptr + iOffset * descriptorSize };
    return D3D12_CPU_DESCRIPTOR_HANDLE{ ptr };
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::OffsetGpuDescHandle(
    D3D12_GPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize { m_descriptorSizes[heapType] };
    const SIZE_T ptr { heapStart.ptr + iOffset * descriptorSize };
    return D3D12_GPU_DESCRIPTOR_HANDLE{ ptr };
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::GetRTVCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_rtvHeap );
    return OffsetCpuDescHandle( m_rtvCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::GetRTVGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_rtvHeap );
    return OffsetGpuDescHandle( m_rtvGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::GetSRVCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_srvHeap );
    return OffsetCpuDescHandle( m_srvCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::GetSRVGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_srvHeap );
    return OffsetGpuDescHandle( m_srvGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::GetSamplerCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_samplerHeap );
    return OffsetCpuDescHandle( m_samplerCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloBundle::GetSamplerGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_samplerHeap );
    return OffsetGpuDescHandle( m_samplerGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, i );
  }

  void DX12AppHelloBundle::CreateRenderTargetViews( Errors& errors )
  {
    if( m_renderTargetInitialized )
      return;

    m_renderTargetInitialized = true;

    auto pSwapChain{ m_swapChain.GetIDXGISwapChain() };
    TAC_ASSERT( pSwapChain );
    TAC_ASSERT( m_device );

    // Create a RTV for each frame.
    for( UINT i{}; i < bufferCount; i++ )
    {
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{ GetRTVCpuDescHandle( i ) };
      PCom< ID3D12Resource >& renderTarget{ m_renderTargets[ i ] };
      TAC_DX12_CALL( m_swapChain->GetBuffer( i, renderTarget.iid(), renderTarget.ppv() ) );
      m_device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      DX12SetName( renderTarget, "Render Target " + ToString( i ) );

      m_renderTargetDescs[ i ] = renderTarget->GetDesc();

      // the render target resource is created in a state that is ready to be displayed on screen
      m_renderTargetStates[ i ] = D3D12_RESOURCE_STATE_PRESENT;
    }

    m_viewport = D3D12_VIEWPORT
    {
     .Width { ( float )m_swapChainDesc.Width },
     .Height { ( float )m_swapChainDesc.Height},
    };

    m_scissorRect = D3D12_RECT
    {
      .right { ( LONG )m_swapChainDesc.Width },
      .bottom { ( LONG )m_swapChainDesc.Height },
    };

    m_viewports = { m_viewport };
    m_scissorRects = { m_scissorRect };
  }

  void DX12AppHelloBundle::TransitionRenderTarget( const int iRT,
                                                    const D3D12_RESOURCE_STATES targetState )
  {
    TransitionParams params
    {
       .mResource { ( ID3D12Resource* )m_renderTargets[ iRT ] },
       .mCurrentState { &m_renderTargetStates[ iRT ] },
       .mTargetState { targetState },
    };

    TransitionResource( params );
  }

  void DX12AppHelloBundle::ResourceBarrier( const D3D12_RESOURCE_BARRIER& barrier )
  {
    // Resource barriers are used to manage resource transitions.

    // ID3D12CommandList::ResourceBarrier
    // - Notifies the driver that it needs to synchronize multiple accesses to resources.
    const Array barriers  { barrier };
    m_commandList->ResourceBarrier( ( UINT )barriers.size(), barriers.data() );
  }

  void DX12AppHelloBundle::PopulateCommandList( Errors& errors )
  {
    // Record all the commands we need to render the scene into the command list.

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

    // However( when ExecuteCommandList() is called on a particular command 
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

    // sets the viewport of the pipeline state's rasterizer state?
    m_commandList->RSSetViewports( (UINT)m_viewports.size(), m_viewports.data() );

    // sets the scissor rect of the pipeline state's rasterizer state?
    m_commandList->RSSetScissorRects( (UINT)m_scissorRects.size(), m_scissorRects.data() );


    // [ ] TODO: comment this function
    const Array descHeaps  {
      ( ID3D12DescriptorHeap* )m_srvHeap,
      ( ID3D12DescriptorHeap* )m_samplerHeap,
    };
    m_commandList->SetDescriptorHeaps( ( UINT )descHeaps.size(), descHeaps.data() );

    // [ ] TODO: comment this function
    {
      // Set the root signature. If it doesn't match, throws an access violation
      m_commandList->SetGraphicsRootSignature( m_rootSignature.Get() );

      // samplers
      m_commandList->SetGraphicsRootDescriptorTable( 0, m_samplerGpuHeapStart );

      // vtx buffers
      {
        const D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor{
          GetSRVGpuDescHandle( SRVIndexes::TriangleVertexBuffer ) };
        m_commandList->SetGraphicsRootDescriptorTable( 1, BaseDescriptor );
      }

      // textures
      {
        const D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor{
          GetSRVGpuDescHandle( SRVIndexes::TriangleTexture ) };

        m_commandList->SetGraphicsRootDescriptorTable( 2, BaseDescriptor );
      }
    }

    static bool recordedBundle;
    if( !recordedBundle )
    {
      const D3D12_DRAW_ARGUMENTS drawArgs
      {
        .VertexCountPerInstance { 3 },
        .InstanceCount { 1 },
        .StartVertexLocation {},
        .StartInstanceLocation {},
      };

#if 0
      m_commandListBundle->Reset( m_commandAllocatorBundle.Get(), nullptr );
      m_commandListBundle->SetPipelineState( ( ID3D12PipelineState* )mPipelineState );
#else

      m_commandListBundle->Reset( m_commandAllocatorBundle.Get(), mPipelineState.Get() );
#endif

      // Root signature... of the pipeline state?... which has already been created with said
      // root signature?
      //
      // You can pass nullptr to unbind the current root signature.
      //
      // Since you can share root signatures between pipelines you only need to set the root sig
      // when that should change
      //
      // for bundles... if this is the same as the root signature of the calling command list,
      // then it will inherit bindings (descriptor tables? constant buffers?)
      //
      // Throws an error if the root signature doesnt match the pso
      m_commandListBundle->SetGraphicsRootSignature( m_rootSignature.Get() );

      m_commandListBundle->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

      m_commandListBundle->DrawInstanced( drawArgs.VertexCountPerInstance,
                                          drawArgs.InstanceCount,
                                          drawArgs.StartVertexLocation,
                                          drawArgs.StartInstanceLocation );



      m_commandListBundle->Close();

      recordedBundle = true;
    }

    // no need to call ID3D12GraphicsCommandList::SetPipelineState( ID3D12PipelineState* ), I think
    // that's implicitly done by ID3D12GraphicsCommandList::Reset( ..., ID3D12PipelineState* )



    // Indicate that the back buffer will be used as a render target.
    TransitionRenderTarget( m_frameIndex, D3D12_RESOURCE_STATE_RENDER_TARGET );

    const Array rtCpuHDescs  { GetRTVCpuDescHandle( m_frameIndex ) };

    m_commandList->OMSetRenderTargets( ( UINT )rtCpuHDescs.size(),
                                       rtCpuHDescs.data(),
                                       false,
                                       nullptr );

    ClearRenderTargetView();


    m_commandList->ExecuteBundle( ( ID3D12GraphicsCommandList* )m_commandListBundle );


    // Indicate that the back buffer will now be used to present.
    //
    // When a back buffer is presented, it must be in the D3D12_RESOURCE_STATE_PRESENT state.
    // If IDXGISwapChain1::Present1 is called on a resource which is not in the PRESENT state,
    // a debug layer warning will be emitted.
    TransitionRenderTarget( m_frameIndex, D3D12_RESOURCE_STATE_PRESENT );

    // Indicates that recording to the command list has finished.
    TAC_DX12_CALL( m_commandList->Close() );
  }

  void DX12AppHelloBundle::ClearRenderTargetView()
  {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVCpuDescHandle( m_frameIndex );

#if 0
    const double speed { 3 };
    const auto t { ( float )Sin( Timestep::GetElapsedTime() * speed ) * 0.5f + 0.5f };

    // Record commands.
    const v4 clearColor  { t, 0.2f, 0.4f, 1.0f };
#else
    const v4 clearColor { v4{ 91, 128, 193, 255.0f } / 255.0f };
#endif
    m_commandList->ClearRenderTargetView( rtvHandle, clearColor.data(), 0, nullptr );
  }

  void DX12AppHelloBundle::ExecuteCommandLists()
  {
    const Array cmdLists{ ( ID3D12CommandList* )m_commandList };

    // Submits an array of command lists for execution.
    m_commandQueue->ExecuteCommandLists( ( UINT )cmdLists.size(), cmdLists.data() );
  }


  void DX12AppHelloBundle::SwapChainPresent( Errors& errors )
  {
    TAC_ASSERT( m_renderTargetInitialized );

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


    TAC_CALL( DXGICheckSwapEffect( m_swapChainDesc.SwapEffect, errors ));

    const DXGI_PRESENT_PARAMETERS params{};

    // For the flip model (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL), values are:
    //   0   - Cancel the remaining time on the previously presented frame
    //         and discard this frame if a newer frame is queued.
    //   1-4 - Synchronize presentation for at least n vertical blanks.
    const UINT SyncInterval { 1 };
    const UINT PresentFlags {};

    // I think this technically adds a frame onto the present queue
    TAC_DX12_CALL( m_swapChain->Present1( SyncInterval, PresentFlags, &params ) );

    // Is this a better place to update the m_frameIndex?
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

  }

  void DX12AppHelloBundle::WaitForPreviousFrame( Errors& errors )
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

  }


  // -----------------------------------------------------------------------------------------------

  // DX12AppHelloBundle

  DX12AppHelloBundle::DX12AppHelloBundle( const Config& cfg ) : App( cfg ) {}

  void DX12AppHelloBundle::Init( Errors& errors )
  {

    AppWindowApi::SetSwapChainAutoCreate( false );
    TAC_CALL( hDesktopWindow = DX12ExampleCreateWindow("DX12 Bundle", errors ) );
  }

  void DX12AppHelloBundle::PreSwapChainInit( Errors& errors)
  {
    static bool didPreSwapChainInit;
    if(didPreSwapChainInit)
      return;

    didPreSwapChainInit = true;

    TAC_CALL( DXGIInit( errors ) );
    TAC_CALL( EnableDebug( errors ) );
    TAC_CALL( CreateDevice( errors ) );
    TAC_CALL( CreateFence( errors ) );
    TAC_CALL( CreateCommandQueue( errors ) );
    TAC_CALL( CreateRTVDescriptorHeap( errors ) );
    TAC_CALL( CreateInfoQueue( errors ) );
    TAC_CALL( CreateSRVDescriptorHeap( errors ) );
    TAC_CALL( CreateCommandAllocator( errors ) );
    TAC_CALL( CreateCommandAllocatorBundle( errors ) );
    TAC_CALL( CreateCommandList( errors ) );
    TAC_CALL( CreateCommandListBundle( errors ) );
    TAC_CALL( CreateRootSignature( errors ) );
    TAC_CALL( CreatePipelineState( errors ) );
    TAC_CALL( CreateSamplerDescriptorHeap( errors ) );
    TAC_CALL( CreateSampler( errors ) );
  }

  void DX12AppHelloBundle::Update( Errors& errors )
  {
  }

  void DX12AppHelloBundle::Render( RenderParams renderParams, Errors& errors )
  {
    if( !AppWindowApi::IsShown( hDesktopWindow ) )
      return;

    TAC_CALL( PreSwapChainInit( errors ) );
    TAC_CALL( DX12CreateSwapChain( errors ) );
    TAC_CALL( CreateRenderTargetViews( errors ) );
    TAC_CALL( CreateVertexBuffer( errors ) );
    TAC_CALL( CreateTexture( errors ) );
    TAC_CALL( PopulateCommandList( errors ) );

    ExecuteCommandLists();

    TAC_CALL( SwapChainPresent( errors ) );
    TAC_CALL( WaitForPreviousFrame( errors ) );
  }

  void DX12AppHelloBundle::Uninit( Errors& errors )
  {

    if( m_commandQueue && m_fence && m_fenceEvent )
      // Ensure that the GPU is no longer referencing resources that are about to be
      // cleaned up by the destructor.
      TAC_CALL( WaitForPreviousFrame( errors ) );

    DXGIUninit();
  }

  App* App::Create()
  {
    const App::Config config
    {
      .mName { "DX12 Hello Bundle" },
      .mDisableRenderer { true },
    };
    return TAC_NEW DX12AppHelloBundle( config );
  };


} // namespace Tac

