#include "tac_dx12_tutorial_6_frame_buf.h" // self-inc

#include "tac_dx12_tutorial_shader_compile.h"
#include "tac_dx12_tutorial_2_dxc.h"
#include "tac_dx12_tutorial_root_sig_builder.h"
#include "tac_dx12_tutorial_input_layout_builder.h"
#include "tac_dx12_tutorial_checkerboard.h"
#include "tac_dx12_tutorial.h"

#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/error/tac_assert.h"
#include "tac-std-lib/dataprocess/tac_text_parser.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_sys_window_api.h"
#include "tac-engine-core/window/tac_window_backend.h"

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-win32/tac_win32.h"

#pragma comment( lib, "d3d12.lib" ) // D3D12...

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

  static const DXGI_FORMAT RTVFormat { DXGI_FORMAT_R16G16B16A16_FLOAT };

  // -----------------------------------------------------------------------------------------------

  // Helper functions for App::Init


  void DX12AppHelloFrameBuf::InitDescriptorSizes()
  {
    for( int i{}; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++ )
      m_descriptorSizes[ i ]
      = m_device->GetDescriptorHandleIncrementSize( ( D3D12_DESCRIPTOR_HEAP_TYPE )i );
  }

  void DX12AppHelloFrameBuf::CreateRTVDescriptorHeap( Errors& errors )
  {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors
    // Descriptors are the primary unit of binding for a single resource in D3D12.

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
    // A descriptor heap is a collection of contiguous allocations of descriptors,
    // one allocation for every descriptor.
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_RTV },
      .NumDescriptors { SWAP_CHAIN_BUFFER_COUNT },
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_rtvHeap.iid(),
                   m_rtvHeap.ppv() ) );
    m_rtvCpuHeapStart = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    m_rtvGpuHeapStart = m_rtvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloFrameBuf::CreateSamplerDescriptorHeap( Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER },
      .NumDescriptors { ( UINT )1 },
      .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_samplerHeap.iid(),
                   m_samplerHeap.ppv() ) );
    m_samplerCpuHeapStart = m_samplerHeap->GetCPUDescriptorHandleForHeapStart();
    m_samplerGpuHeapStart = m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloFrameBuf::CreateSRVDescriptorHeap( Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type           { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV },
      .NumDescriptors { ( UINT )SRVIndexes::Count },
      .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
    };
    TAC_DX12_CALL( m_device->CreateDescriptorHeap(
                   &desc,
                   m_srvHeap.iid(),
                   m_srvHeap.ppv() ) );
    m_srvCpuHeapStart = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvGpuHeapStart = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
  }

  void DX12AppHelloFrameBuf::CreateBufferSRV( Errors& errors )
  {
    TAC_ASSERT( m_vertexBuffer );

    // srv --> byteaddressbuffer
    // uav --> rwbyteaddressbuffer

    const D3D12_BUFFER_SRV Buffer
    {
      .FirstElement  {},
      .NumElements   { m_vertexBufferByteCount / 4 },
      .Flags         { D3D12_BUFFER_SRV_FLAG_RAW }, // for byteaddressbuffer
    };

    const D3D12_SHADER_RESOURCE_VIEW_DESC Desc
    {
      .Format                  { DXGI_FORMAT_R32_TYPELESS }, // for byteaddressbuffer
      .ViewDimension           { D3D12_SRV_DIMENSION_BUFFER },
      .Shader4ComponentMapping { D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING }, // swizzling?
      .Buffer                  { Buffer },
    };


     const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{
       GetSRVCpuDescHandle( SRVIndexes::TriangleVertexBuffer ) };
    
    m_device->CreateShaderResourceView( ( ID3D12Resource* )m_vertexBuffer,
                                        &Desc,
                                        DestDescriptor );
  }

  void DX12AppHelloFrameBuf::CreateSampler( Errors& errors )
  {
    const D3D12_SAMPLER_DESC Desc
    {
      .Filter         { D3D12_FILTER_MIN_MAG_MIP_LINEAR }, // D3D12_FILTER_MIN_MAG_MIP_POINT,
      .AddressU       { D3D12_TEXTURE_ADDRESS_MODE_WRAP },
      .AddressV       { D3D12_TEXTURE_ADDRESS_MODE_WRAP },
      .AddressW       { D3D12_TEXTURE_ADDRESS_MODE_WRAP },
      .ComparisonFunc { D3D12_COMPARISON_FUNC_NEVER },
      .MinLOD         {},
      .MaxLOD         { D3D12_FLOAT32_MAX },
    };
    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = GetSamplerCpuDescHandle( 0 );
    m_device->CreateSampler( &Desc, DestDescriptor );
  }

  void DX12AppHelloFrameBuf::CreateTexture( Errors& errors )
  {
    static bool sCreated;
    TAC_ASSERT( !sCreated );
    sCreated = true;

    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    PCom<ID3D12Resource> textureUploadHeap;


    // Create the texture.

    const DXGI_SAMPLE_DESC SampleDesc
    {
      .Count   { 1 },
      .Quality {},
    };

    // Describe and create a Texture2D.
    const D3D12_RESOURCE_DESC resourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_TEXTURE2D },
      .Width            { Checkerboard::TextureWidth },
      .Height           { Checkerboard::TextureHeight },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { DXGI_FORMAT_R8G8B8A8_UNORM },
      .SampleDesc       {SampleDesc },
    };

    m_textureDesc = resourceDesc;

    const D3D12_HEAP_PROPERTIES defaultHeapProps{ .Type { D3D12_HEAP_TYPE_DEFAULT }, };

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

    const D3D12_HEAP_PROPERTIES uploadHeapProps{ .Type { D3D12_HEAP_TYPE_UPLOAD }, };


    const D3D12_RESOURCE_DESC uploadBufferResourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_BUFFER },
      .Width            { totalBytes },
      .Height           { 1 },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .SampleDesc       { SampleDesc },
      .Layout           { D3D12_TEXTURE_LAYOUT_ROW_MAJOR },
    };

    TAC_CALL( DX12ExampleContextScope context{ mContextManager.GetContext( errors ) } );
    ID3D12GraphicsCommandList* m_commandList { context.GetCommandList() };

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
    const Vector<UINT8> texture { Checkerboard::GenerateCheckerboardTextureData() };
    const D3D12_SUBRESOURCE_DATA textureData
    {
      .pData      { texture.data() },
      .RowPitch   { Checkerboard::TexturePixelSize * Checkerboard::TextureWidth },
      .SlicePitch { Checkerboard::TexturePixelSize * Checkerboard::TextureWidth * Checkerboard::TextureHeight },
    };

    // --- update subresource begin ---

    const int nSubRes = 1;
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
    for( int iSubRes = 0; iSubRes < nSubRes; ++iSubRes )
    {

      TAC_ASSERT( totalBytes >= requiredByteCount );

      const D3D12_RANGE readRange{}; // not reading from CPU
      void* mappedData;
      TAC_DX12_CALL( textureUploadHeap->Map( ( UINT )iSubRes, &readRange, &mappedData ) );

      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout { footprints[ iSubRes ] };
      const UINT rowCount { rowCounts[ iSubRes] };

      const D3D12_MEMCPY_DEST DestData 
      {
        .pData      { (char*)mappedData + layout.Offset },
        .RowPitch   { layout.Footprint.RowPitch },
        .SlicePitch { SIZE_T( layout.Footprint.RowPitch ) * SIZE_T( rowCount ) },
      };

      const int rowByteCount { ( int )rowByteCounts[ iSubRes ] };

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


      textureUploadHeap->Unmap( iSubRes, nullptr);
    }


    for( int iSubRes = 0; iSubRes < nSubRes; ++iSubRes )
    {
      const D3D12_TEXTURE_COPY_LOCATION Dst
      {
        .pResource        { ( ID3D12Resource* )m_texture },
        .Type             { D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX },
        .SubresourceIndex { ( UINT )iSubRes },
      };

      const D3D12_TEXTURE_COPY_LOCATION Src
      {
        .pResource       { ( ID3D12Resource* )textureUploadHeap },
        .Type            { D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT },
        .PlacedFootprint { footprints[ iSubRes ] },
      };

      m_commandList->CopyTextureRegion( &Dst, 0, 0, 0, &Src, nullptr );
    }

    // --- update subresource end ---

    const TransitionParams transitionParams
    {
       .mResource     { ( ID3D12Resource* )m_texture },
       .mCurrentState { &m_textureResourceStates },
       .mTargetState  { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
    };

    TransitionResource( m_commandList, transitionParams );

    D3D12_TEX2D_SRV Texture2D{ .MipLevels { 1 }, };

    // Describe and create a SRV for the texture.
    const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
    {
      .Format                  { resourceDesc.Format },
      .ViewDimension           { D3D12_SRV_DIMENSION_TEXTURE2D },
      .Shader4ComponentMapping { D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING },
      .Texture2D               { Texture2D },
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor { GetSRVCpuDescHandle( SRVIndexes::TriangleTexture ) };
    m_device->CreateShaderResourceView( ( ID3D12Resource* )m_texture.Get(),
                                        &srvDesc,
                                        DestDescriptor );


    // wait until assets have been uploaded to the GPU.
    // Wait for the command list to execute; we are reusing the same command 
    // list in our main loop but for now, we just want to wait for setup to 
    // complete before continuing.
    context.ExecuteSynchronously();
  }

  void DX12AppHelloFrameBuf::TransitionResource( ID3D12GraphicsCommandList* m_commandList,
                                                 TransitionParams params )
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
      .Type        { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
      .Transition  { Transition },
    };

    ResourceBarrier( m_commandList, barrier );

    *params.mCurrentState = params.mTargetState;
  }


  void DX12AppHelloFrameBuf::CreateCommandAllocatorBundle( Errors& errors )
  {
    // a command allocator manages storage for cmd lists and bundles
    TAC_ASSERT( m_device );
    TAC_DX12_CALL( m_device->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_BUNDLE,
      m_commandAllocatorBundle.iid(),
      m_commandAllocatorBundle.ppv() ) );
    DX12SetName( m_commandAllocatorBundle, "Bundle Allocator" );
  }

  void DX12AppHelloFrameBuf::CreateCommandListBundle( Errors& errors )
  {
    // Create the command list
    //
    // Note: CreateCommandList1 creates it the command list in a closed state, as opposed to
    //       CreateCommandList, which creates in a open state.
    PCom< ID3D12CommandList > commandList;
    TAC_DX12_CALL( m_device->CreateCommandList1( 0,
                                                 D3D12_COMMAND_LIST_TYPE_BUNDLE,
                                                 D3D12_COMMAND_LIST_FLAG_NONE,
                                                 commandList.iid(),
                                                 commandList.ppv() ) );

    TAC_ASSERT( commandList );
    commandList.QueryInterface( m_commandListBundle );
    TAC_ASSERT( m_commandListBundle );
    DX12SetName( m_commandListBundle, "my_bundle_cmdlist" );
  }

  void DX12AppHelloFrameBuf::CreateBuffer( Errors& errors )
  {
    static bool sInitialized;
    TAC_ASSERT(!sInitialized);
    sInitialized = true;

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

    const DXGI_SAMPLE_DESC SampleDesc
    {
        .Count   { 1 },
        .Quality {},
    };

    // used for both default and upload heaps
    const D3D12_RESOURCE_DESC resourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_BUFFER },
      .Alignment        {},
      .Width            { m_vertexBufferByteCount },
      .Height           { 1 },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { DXGI_FORMAT_UNKNOWN },
      .SampleDesc       { SampleDesc },
      .Layout           { D3D12_TEXTURE_LAYOUT_ROW_MAJOR },
    };

    TAC_CALL( DX12ExampleContextScope context{ mContextManager.GetContext( errors ) } );
    ID3D12GraphicsCommandList* m_commandList { context.GetCommandList() };

    // Create default heap
    D3D12_RESOURCE_STATES defaultHeapState { D3D12_RESOURCE_STATE_COMMON };
    {

      const D3D12_HEAP_PROPERTIES defaultHeapProps{ .Type { D3D12_HEAP_TYPE_DEFAULT }, };

      // we want to copy into here from the upload buffer
      //D3D12_RESOURCE_STATES defaultHeapState = D3D12_RESOURCE_STATE_COPY_DEST;
      //
      // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html#initial-resource-state
      // Despite the fact that legacy resource creation API’s have an Initial State,
      // buffers do not have a layout, and thus are treated as though they have an initial state of
      // D3D12_RESOURCE_STATE_COMMON. 
      // ... am I using a legacy API?
      // ID3D12Device10::CreateCommittedResource3 uses a D3D12_BARRIER_LAYOUT 
      // instead of a D3D12_RESOURCE_STATES

      // Creates both a resource and an implicit heap,
      // such that the heap is big enough to contain the entire resource,
      // and the resource is mapped to the heap.
      TAC_CALL( m_device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        defaultHeapState,
        nullptr,
        m_vertexBuffer.iid(),
        m_vertexBuffer.ppv() ) );

      DX12SetName( m_vertexBuffer.Get(), "vtxbuf" );

    }


    // Copy the triangle data to the vertex buffer upload heap.
    TAC_CALL( DX12ExampleGPUUploadAllocator::DynAlloc alloc{
      context.mContext.mGPUUploadAllocator.Allocate( m_vertexBufferByteCount, errors ) }  );
    MemCpy( alloc.mCPUAddr, triangleVertices, m_vertexBufferByteCount );


    // copy the vertex buffer
    {
      const TransitionParams transition1
      {
        .mResource     { (ID3D12Resource*)m_vertexBuffer },
        .mCurrentState { &defaultHeapState },

        // for the vertex shader byteaddressbuffer
        .mTargetState  { D3D12_RESOURCE_STATE_COPY_DEST },
      };
      TransitionResource( m_commandList, transition1 );

      m_commandList->CopyBufferRegion( ( ID3D12Resource* )m_vertexBuffer, // dst
                                       0, // dstoffest
                                       alloc.mResource, // src
                                       alloc.mResourceOffest, // srcoffset
                                       m_vertexBufferByteCount );

      const TransitionParams transitionParams
      {
        .mResource     { (ID3D12Resource*)m_vertexBuffer },
        .mCurrentState { &defaultHeapState },

        // for the vertex shader byteaddressbuffer
        .mTargetState  { D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE },
      };
      TransitionResource( m_commandList, transitionParams );
    }

    // wait until assets have been uploaded to the GPU.
    // Wait for the command list to execute; we are reusing the same command 
    // list in our main loop but for now, we just want to wait for setup to 
    // complete before continuing.
    context.ExecuteSynchronously();

    TAC_CALL( CreateBufferSRV( errors ) );
  }

  void DX12AppHelloFrameBuf::CreateRootSignature( Errors& errors )
  {
    DX12ExampleRootSignatureBuilder builder( ( ID3D12Device* )m_device );

    // register(s0)  samplers
    builder.AddRootDescriptorTable( D3D12_SHADER_VISIBILITY_PIXEL,
                                    D3D12_DESCRIPTOR_RANGE1{
                                      .RangeType      { D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER },
                                      .NumDescriptors { 1 } } );

    // register(t0+, space0) vertex buffers
    builder.AddRootDescriptorTable( D3D12_SHADER_VISIBILITY_VERTEX,
                                    D3D12_DESCRIPTOR_RANGE1{
                                      .RangeType          { D3D12_DESCRIPTOR_RANGE_TYPE_SRV },
                                      .NumDescriptors     { 1 },
                                      .BaseShaderRegister {},
                                      .RegisterSpace      {}, } );

    // register(t0+, space1) textures
    builder.AddRootDescriptorTable( D3D12_SHADER_VISIBILITY_PIXEL,
                                    D3D12_DESCRIPTOR_RANGE1{
                                      .RangeType          { D3D12_DESCRIPTOR_RANGE_TYPE_SRV },
                                      .NumDescriptors     { 1 },
                                      .BaseShaderRegister {},
                                      .RegisterSpace      { 1 }, } );

    builder.AddConstantBuffer( D3D12_SHADER_VISIBILITY_ALL,
                               D3D12_ROOT_DESCRIPTOR1
                               {
                                  .ShaderRegister {},
                                  .RegisterSpace  {},
                               } );

    m_rootSignature = TAC_CALL( builder.Build( errors ) );
    DX12SetName( m_rootSignature, "My Root Signature" );
  }

  void DX12AppHelloFrameBuf::CreatePipelineState( Errors& errors )
  {
    const AssetPathStringView shaderAssetPath = "assets/hlsl/DX12HelloFrameBuf.hlsl";

    const DX12ExampleProgramCompiler::Params compilerParams
    {
      .mOutputDir { sShellPrefPath },
      .mDevice    { ( ID3D12Device* )m_device },
    };
    TAC_CALL( DX12ExampleProgramCompiler compiler( compilerParams, errors ) );

    DX12ExampleProgramCompiler::Result compileResult = TAC_CALL( compiler.Compile( shaderAssetPath, errors ) );


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

    VertexDeclarations vtxDecls;
    vtxDecls.push_back( posDecl );
    vtxDecls.push_back( colDecl );

    const DX12BuiltInputLayout inputLayout{ vtxDecls };

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

    const D3D12_RENDER_TARGET_BLEND_DESC blendDesc
    {
      .RenderTargetWriteMask { D3D12_COLOR_WRITE_ENABLE_ALL },
    };

    const D3D12_BLEND_DESC BlendState
    {
      .RenderTarget { blendDesc },
    };


    const DXGI_SAMPLE_DESC SampleDesc{ .Count { 1 } };
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
    {
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

  void DX12AppHelloFrameBuf::DX12CreateSwapChain(  const SysWindowApi windowApi, Errors& errors )
  {
    TAC_ASSERT( !m_swapChainValid );

    const auto hwnd { ( HWND ) windowApi.GetNWH( hDesktopWindow ) };
    if( !hwnd )
      return;

    ID3D12CommandQueue* commandQueue { mCommandQueue.GetCommandQueue() };
    TAC_ASSERT( commandQueue );

    const v2i size { windowApi.GetSize( hDesktopWindow ) };
    const DXGISwapChainWrapper::Params scInfo
    {
      .mHwnd        { hwnd },
      .mDevice      { ( IUnknown* )commandQueue }, // swap chain can force flush the queue
      .mBufferCount { SWAP_CHAIN_BUFFER_COUNT },
      .mWidth       { size.x },
      .mHeight      { size.y },
    };

    TAC_CALL( m_swapChain.Init( scInfo, errors ) );
    TAC_CALL( m_swapChain->GetDesc1( &m_swapChainDesc ) );
    m_swapChainValid = true;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::OffsetCpuDescHandle(
    D3D12_CPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize { m_descriptorSizes[heapType] };
    const SIZE_T ptr { heapStart.ptr + iOffset * descriptorSize };
    return D3D12_CPU_DESCRIPTOR_HANDLE{ ptr };
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::OffsetGpuDescHandle(
    D3D12_GPU_DESCRIPTOR_HANDLE heapStart,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    int iOffset ) const
  {
    const UINT descriptorSize { m_descriptorSizes[heapType] };
    const SIZE_T ptr { heapStart.ptr + iOffset * descriptorSize };
    return D3D12_GPU_DESCRIPTOR_HANDLE{ ptr };
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::GetRTVCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_rtvHeap );
    return OffsetCpuDescHandle( m_rtvCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::GetRTVGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_rtvHeap );
    return OffsetGpuDescHandle( m_rtvGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, i );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::GetSRVCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_srvHeap );
    return OffsetCpuDescHandle( m_srvCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::GetSRVGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_srvHeap );
    return OffsetGpuDescHandle( m_srvGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, i );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::GetSamplerCpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_samplerHeap );
    return OffsetCpuDescHandle( m_samplerCpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, i );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12AppHelloFrameBuf::GetSamplerGpuDescHandle( int i ) const
  {
    TAC_ASSERT( m_samplerHeap );
    return OffsetGpuDescHandle( m_samplerGpuHeapStart, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, i );
  }

  void DX12AppHelloFrameBuf::CreateRenderTargetViews( Errors& errors )
  {
    TAC_ASSERT( !m_renderTargetInitialized );
    m_renderTargetInitialized = true; // dont delete this var, its used elsewhere

    auto pSwapChain{ m_swapChain.GetIDXGISwapChain() };
    TAC_ASSERT( pSwapChain );
    TAC_ASSERT( m_device );

    // Create a RTV for each frame.
    for( UINT i{}; i < SWAP_CHAIN_BUFFER_COUNT; i++ )
    {
      const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle { GetRTVCpuDescHandle( i ) };
      PCom< ID3D12Resource >& renderTarget { m_renderTargets[ i ] };
      TAC_DX12_CALL( m_swapChain->GetBuffer( i, renderTarget.iid(), renderTarget.ppv() ) );
      m_device->CreateRenderTargetView( ( ID3D12Resource* )renderTarget, nullptr, rtvHandle );

      DX12SetName( renderTarget, "Render Target " + ToString( i ) );

      m_renderTargetDescs[ i ] = renderTarget->GetDesc();

      // the render target resource is created in a state that is ready to be displayed on screen
      m_renderTargetStates[ i ] = D3D12_RESOURCE_STATE_PRESENT;
    }

    m_viewport = D3D12_VIEWPORT
    {
      .Width  { ( float )m_swapChainDesc.Width },
      .Height { ( float )m_swapChainDesc.Height },
    };

    m_scissorRect = D3D12_RECT
    {
      .right  { ( LONG )m_swapChainDesc.Width },
      .bottom { ( LONG )m_swapChainDesc.Height },
    };

    m_viewports =    { m_viewport };
    m_scissorRects = { m_scissorRect };
  }


  void DX12AppHelloFrameBuf::TransitionRenderTarget( ID3D12GraphicsCommandList* m_commandList,
                                                     const int iRT,
                                                     const D3D12_RESOURCE_STATES targetState )
  {
    TransitionParams params
    {
       .mResource     { ( ID3D12Resource* )m_renderTargets[ iRT ] },
       .mCurrentState { &m_renderTargetStates[ iRT ] },
       .mTargetState  { targetState },
    };

    TransitionResource( m_commandList, params );
  }

  void DX12AppHelloFrameBuf::ResourceBarrier( ID3D12GraphicsCommandList* m_commandList,
                                              const D3D12_RESOURCE_BARRIER& barrier )
  {
    // Resource barriers are used to manage resource transitions.

    // ID3D12CommandList::ResourceBarrier
    // - Notifies the driver that it needs to synchronize multiple accesses to resources.
    const Array barriers { barrier };
    m_commandList->ResourceBarrier( ( UINT )barriers.size(), barriers.data() );
  }

  void DX12AppHelloFrameBuf::RecordBundle()
  {
    static bool sRecorded;
    TAC_ASSERT( !sRecorded );
    sRecorded = true;

    const D3D12_DRAW_ARGUMENTS drawArgs
    {
      .VertexCountPerInstance { 3 },
      .InstanceCount          { 1 },
      .StartVertexLocation    {},
      .StartInstanceLocation  {},
    };

    m_commandListBundle->Reset( m_commandAllocatorBundle.Get(), mPipelineState.Get() );

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
  }

  void DX12AppHelloFrameBuf::PopulateCommandList( DX12ExampleContextScope& context,
                                                  float translateX,
                                                  float translateY,
                                                  float scale,
                                                  Errors& errors )
  {
    // Record all the commands we need to render the scene into the command list.

    ID3D12GraphicsCommandList* m_commandList { context.GetCommandList() };



    // sets the viewport of the pipeline state's rasterizer state?
    m_commandList->RSSetViewports( ( UINT )m_viewports.size(), m_viewports.data() );

    // sets the scissor rect of the pipeline state's rasterizer state?
    m_commandList->RSSetScissorRects( ( UINT )m_scissorRects.size(), m_scissorRects.data() );

    const Array descHeaps{
      ( ID3D12DescriptorHeap* )m_srvHeap,
      ( ID3D12DescriptorHeap* )m_samplerHeap,
    };
    m_commandList->SetDescriptorHeaps( ( UINT )descHeaps.size(), descHeaps.data() );

    // The TriangleVertexBuffer and TriangleTexture SRVs both live in the m_srvHeap.
    // Connect ranges from the srv heap to descriptor tables within the root signature
    {
      // Set the root signature. If it doesn't match, throws an access violation
      m_commandList->SetGraphicsRootSignature( m_rootSignature.Get() );

      // samplers
      m_commandList->SetGraphicsRootDescriptorTable( 0, m_samplerGpuHeapStart );

      // vtx buffers
      {
        const D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor =
          GetSRVGpuDescHandle( SRVIndexes::TriangleVertexBuffer );
        m_commandList->SetGraphicsRootDescriptorTable( 1, BaseDescriptor );
      }

      // textures
      {
        const D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor =
          GetSRVGpuDescHandle( SRVIndexes::TriangleTexture );

        m_commandList->SetGraphicsRootDescriptorTable( 2, BaseDescriptor );
      }

      // Constant Buffer
      {

        // must match MyCBufType in hlsl
        struct MyCBufType
        {
          m4           mWorld;
          u32          mVertexBuffer;
          u32          mTexture;
        };

        const m4 transform { m4::Translate( v3( translateX, translateY, 0 ) ) * m4::Scale( v3( scale ) ) };
        const MyCBufType cbuf
        {
          .mWorld        { transform },
          .mVertexBuffer {},
          .mTexture      {},
        };

        // So I was supposed to learn about D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT from
        // https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources
        // maybe

        const int byteCount{ RoundUpToNearestMultiple(
          sizeof( MyCBufType ),
          D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT ) };

        DX12ExampleGPUUploadAllocator& mUploadAllocator { context.mContext.mGPUUploadAllocator };

        TAC_CALL( DX12ExampleGPUUploadAllocator::DynAlloc allocation{
          mUploadAllocator.Allocate( byteCount, errors ) }  );

        MemCpy( allocation.mCPUAddr, &cbuf, sizeof( MyCBufType ) );

        const D3D12_GPU_VIRTUAL_ADDRESS BufferLocation { allocation.mGPUAddr };

        // Here we are using SetGraphicsRootConstantBufferView to set a root CBV, as opposed to
        // using SetGraphicsRootDescriptorTable to set a table containing an element 
        // (ID3D12Device::CreateBufferView) which describes a CBV.
        m_commandList->SetGraphicsRootConstantBufferView( 3, BufferLocation );
      }
    }


    // no need to call ID3D12GraphicsCommandList::SetPipelineState( ID3D12PipelineState* ), I think
    // that's implicitly done by ID3D12GraphicsCommandList::Reset( ..., ID3D12PipelineState* )

    m_commandList->ExecuteBundle( ( ID3D12GraphicsCommandList* )m_commandListBundle );
  }

  void DX12AppHelloFrameBuf::ClearRenderTargetView( ID3D12GraphicsCommandList* m_commandList )
  {
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVCpuDescHandle( m_backbufferIndex );

    const v4 clearColor { v4{ 91, 128, 193, 255.0f } / 255.0f };
    m_commandList->ClearRenderTargetView( rtvHandle, clearColor.data(), 0, nullptr );
  }

  void DX12AppHelloFrameBuf::SwapChainPresent( Errors& errors )
  {
    TAC_ASSERT( m_renderTargetInitialized );

    // Present the frame.

    // Q: What does it mean to 'present' the frame?
    //    ... it is added to the present queue(?)


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

  // -----------------------------------------------------------------------------------------------

  // DX12AppHelloFrameBuf

  DX12AppHelloFrameBuf::DX12AppHelloFrameBuf( const Config& cfg ) : App( cfg ) {}

  void         DX12AppHelloFrameBuf::Init( InitParams initParams, Errors& errors )
  {
    const SysWindowApi windowApi{ initParams.mWindowApi };
    windowApi.SetSwapChainAutoCreate( false );

    TAC_CALL( hDesktopWindow = DX12ExampleCreateWindow( windowApi, "DX12 Frame Buf", errors ) );
  }

  void         DX12AppHelloFrameBuf::PreSwapChainInit( Errors& errors)
  {
    TAC_CALL( DXGIInit( errors ) );

    DX12ExampleDebugLayer debugLayer;
    TAC_CALL( debugLayer.Init( errors ) );

    DX12ExampleDevice deviceInitializer;
    TAC_CALL( deviceInitializer.Init( debugLayer, errors ) );

    m_device0 = deviceInitializer.mDevice;
    m_device = m_device0.QueryInterface< ID3D12Device5 >();
    m_debugDevice0 = deviceInitializer.mDebugDevice;
    m_debugDevice = m_debugDevice0.QueryInterface<ID3D12DebugDevice2 >();

    DX12ExampleInfoQueue infoQueue;
    TAC_CALL( infoQueue.Init( debugLayer, m_device.Get(), errors ) );

    InitDescriptorSizes();
    TAC_CALL( mCommandQueue.Create( m_device.Get(), errors ) );
    TAC_CALL( CreateRTVDescriptorHeap( errors ) );
    TAC_CALL( CreateSRVDescriptorHeap( errors ) );
    TAC_CALL( CreateCommandAllocatorBundle( errors ) );
    TAC_CALL( CreateCommandListBundle( errors ) );
    TAC_CALL( CreateRootSignature( errors ) );
    TAC_CALL( CreatePipelineState( errors ) );
    TAC_CALL( CreateSamplerDescriptorHeap( errors ) );
    TAC_CALL( CreateSampler( errors ) );

    mCommandAllocatorPool.Init( m_device0, &mCommandQueue );
    mContextManager.Init( &mCommandAllocatorPool,
                          &mCommandQueue,
                          &mUploadPageManager,
                          m_device0 );

    mUploadPageManager.Init( m_device.Get(), &mCommandQueue );
  }

  void         DX12AppHelloFrameBuf::Update( UpdateParams updateParams, Errors& errors )
  {
    if( !updateParams.mWindowApi.IsShown( hDesktopWindow ) )
      return;

    const double t { Timestep::GetElapsedTime().mSeconds };
    mState.mTranslateX = ( float )Sin( t * 0.2f );
  }

  void         DX12AppHelloFrameBuf::Uninit( Errors& errors )
  {

      // Ensure that the GPU is no longer referencing resources that are about to be
      // cleaned up by the destructor.
    TAC_CALL( mCommandQueue.WaitForIdle( errors ) );

    DXGIUninit();
  }

  App::IState* DX12AppHelloFrameBuf::GetGameState()
  {
    State* state { TAC_NEW State };
    *state = mState;
    return state;
  }

  void         DX12AppHelloFrameBuf::RenderBegin( Errors& errors )
  {
    TAC_ASSERT_INDEX( m_gpuFlightFrameIndex, MAX_GPU_FRAME_COUNT );

    const FenceSignal signalValue { mFenceValues[ m_gpuFlightFrameIndex ] };
    TAC_CALL( mCommandQueue.WaitForFence( signalValue, errors ) );
  }

  void         DX12AppHelloFrameBuf::RenderEnd( Errors& errors )
  {
    mFenceValues[ m_gpuFlightFrameIndex ] = TAC_CALL( mCommandQueue.IncrementFence( errors ) );
    TAC_CALL( SwapChainPresent( errors ) );

    m_backbufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    mSentGPUFrameCount++;
    ++m_gpuFlightFrameIndex %= MAX_GPU_FRAME_COUNT;

    TAC_ASSERT( m_backbufferIndex == mSentGPUFrameCount % SWAP_CHAIN_BUFFER_COUNT );
    TAC_ASSERT( m_gpuFlightFrameIndex == mSentGPUFrameCount % MAX_GPU_FRAME_COUNT );
  }
   
  void         DX12AppHelloFrameBuf::Render( RenderParams renderParams, Errors& errors )
  {
    const SysWindowApi windowApi { renderParams.mWindowApi };

    const State* oldState { ( State* )renderParams.mOldState };
    const State* newState { ( State* )renderParams.mNewState };
    const float t { renderParams.mT };

    const float translateX { Lerp( oldState->mTranslateX, newState->mTranslateX, t ) };

    static bool once;
    if( !once )
    {
      once = true;
      TAC_CALL( PreSwapChainInit( errors ) );
      TAC_CALL( DX12CreateSwapChain( windowApi, errors ) );
      TAC_CALL( CreateRenderTargetViews( errors ) );
      TAC_CALL( CreateBuffer( errors ) );
      TAC_CALL( CreateTexture( errors ) );
      RecordBundle();
    }

    if( !windowApi.IsShown( hDesktopWindow ) )
      return;

    TAC_CALL( RenderBegin( errors ) );

    {
      DX12ExampleContextScope context = TAC_CALL( mContextManager.GetContext( errors ) );
      ID3D12GraphicsCommandList* m_commandList = context.GetCommandList();

      // The associated pipeline state (IA, OM, RS, ... )
      // that the command list will modify, all leading to a draw call?
      m_commandList->SetPipelineState( ( ID3D12PipelineState* )mPipelineState );


      // Indicate that the back buffer will be used as a render target.
      TransitionRenderTarget( m_commandList, m_backbufferIndex, D3D12_RESOURCE_STATE_RENDER_TARGET );

      const Array rtCpuHDescs = { GetRTVCpuDescHandle( m_backbufferIndex ) };

      m_commandList->OMSetRenderTargets( ( UINT )rtCpuHDescs.size(),
                                         rtCpuHDescs.data(),
                                         false,
                                         nullptr );

      ClearRenderTargetView( m_commandList );


      // note: y is up
      float curTranslateX { translateX };
      float curTranslateY {};
      float curScale { 0.10f };

      float oldTranslateX { oldState->mTranslateX };
      static float oldTranslateY { -0.1f };
      static float oldScale { 0.2f }; // -0.5f;

      float newTranslateX { newState->mTranslateX };
      static float newTranslateY { 0.1f };
      static float newScale { 0.5f }; // 0.5f;

      static float prevOldTranslateX { oldTranslateX };
      if( prevOldTranslateX != oldTranslateX )
      {
        prevOldTranslateX = oldTranslateX;
        Swap( oldScale, newScale );
        Swap( oldTranslateY, newTranslateY );
      }

      TAC_CALL( PopulateCommandList( context, oldTranslateX, oldTranslateY, oldScale, errors ) );
      TAC_CALL( PopulateCommandList( context, newTranslateX, newTranslateY, newScale, errors ) );
      TAC_CALL( PopulateCommandList( context, curTranslateX, curTranslateY, curScale, errors ) );


      // Indicate that the back buffer will now be used to present.
      //
      // When a back buffer is presented, it must be in the D3D12_RESOURCE_STATE_PRESENT state.
      // If IDXGISwapChain1::Present1 is called on a resource which is not in the PRESENT state,
      // a debug layer warning will be emitted.
      TransitionRenderTarget( m_commandList, m_backbufferIndex, D3D12_RESOURCE_STATE_PRESENT );
    }
    TAC_CALL( RenderEnd( errors ) );
  }

  // -----------------------------------------------------------------------------------------------

  App* App::Create()
  {
    const App::Config config
    {
      .mName            { "DX12 Hello Frame Buf" },
      .mDisableRenderer { true },
    };
    return TAC_NEW DX12AppHelloFrameBuf( config );
  };

} // namespace Tac

