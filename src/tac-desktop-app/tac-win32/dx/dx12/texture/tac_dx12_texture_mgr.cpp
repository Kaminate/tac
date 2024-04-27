#include "tac_dx12_texture_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"

namespace Tac::Render
{
  void DX12TextureMgr::Init( Params params )
  {
    mDevice = params.mDevice;
    mCpuDescriptorHeapRTV = params.mCpuDescriptorHeapRTV;
  }

  void DX12TextureMgr::CreateTexture( TextureHandle h, CreateTextureParams params , Errors& errors)
  {
    TAC_ASSERT_UNIMPLEMENTED;

    DX12Texture* texture = FindTexture( h );
    TAC_ASSERT( texture );












    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    PCom<ID3D12Resource> textureUploadHeap;


    // Create the texture.

    const DXGI_SAMPLE_DESC SampleDesc { .Count = 1, .Quality = 0 };

    // Describe and create a Texture2D.
    const D3D12_RESOURCE_DESC resourceDesc =
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
    m_device->GetCopyableFootprints( &resourceDesc,
                                     0,
                                     1,
                                     0,
                                     nullptr,
                                     nullptr,
                                     nullptr,
                                     &totalBytes );

    const D3D12_HEAP_PROPERTIES uploadHeapProps { .Type = D3D12_HEAP_TYPE_UPLOAD, };

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
      .pData      { texture.data() },
      .RowPitch   { Checkerboard::TexturePixelSize * Checkerboard::TextureWidth },
      .SlicePitch { Checkerboard::TexturePixelSize * Checkerboard::TextureWidth * Checkerboard::TextureHeight },
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
    for( int subresourceIndex { 0 }; subresourceIndex < nSubRes; ++subresourceIndex )
    {

      TAC_ASSERT( totalBytes >= requiredByteCount );

      const D3D12_RANGE readRange{}; // not reading from CPU
      void* mappedData;
      TAC_DX12_CALL( textureUploadHeap->Map( (UINT)subresourceIndex, &readRange, &mappedData ) );

      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = footprints[ subresourceIndex ];
      const UINT rowCount { rowCounts[ subresourceIndex] };

      const D3D12_MEMCPY_DEST DestData 
      {
        .pData { (char*)mappedData + layout.Offset },
        .RowPitch { layout.Footprint.RowPitch },
        .SlicePitch { SIZE_T( layout.Footprint.RowPitch ) * SIZE_T( rowCount ) },
      };

      const int rowByteCount { ( int )rowByteCounts[ subresourceIndex ] };

      const UINT NumSlices { layout.Footprint.Depth };

      // For each slice
      for (UINT z { 0 }; z < NumSlices; ++z)
      {
          auto pDestSlice { (BYTE*)DestData.pData + DestData.SlicePitch * z };
          auto pSrcSlice { (const BYTE*)textureData.pData + textureData.SlicePitch * LONG_PTR(z) };
          for (UINT y { 0 }; y < rowCount; ++y)
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


    for( int iSubRes { 0 }; iSubRes < nSubRes; ++iSubRes )
    {
        const D3D12_TEXTURE_COPY_LOCATION Dst
        {
          .pResource        { (ID3D12Resource *)m_texture },
          .Type             { D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX },
          .SubresourceIndex { (UINT)iSubRes },
        };

        const D3D12_TEXTURE_COPY_LOCATION Src
        {
          .pResource       { (ID3D12Resource *)textureUploadHeap },
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

    TransitionResource( transitionParams );

    // Describe and create a SRV for the texture.
    const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc  {
      .Format = resourceDesc.Format,
      .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
      .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
      .Texture2D = D3D12_TEX2D_SRV { .MipLevels = 1, },
    };

    const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{
      GetSRVCpuDescHandle( SRVIndexes::TriangleTexture ) };
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






















    if( params.mBinding & Binding::DepthStencil )
    {
      const D3D12_TEX2D_DSV depthTex2D
      {
      };

      DXGI_FORMAT_R16G16B16A16_FLOAT;

      const DXGI_FORMAT Format = TexFmtToDxgiFormat( params.mf;

      const D3D12_DEPTH_STENCIL_VIEW_DESC desc
      {
        .Format{Format},
        .ViewDimension{ D3D12_DSV_DIMENSION_TEXTURE2D },
      };
      ID3D12Resource* pResource = ;
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor = ;
      mDevice->CreateDepthStencilView( pResource, &desc, descDescriptor );
    }

    *texture = DX12Texture
    {
      .mResource { resource },
      .mDesc     { resource->GetDesc() },
      .mState    { D3D12_RESOURCE_STATE_PRESENT }, // Render targets are created in present state
      .mRTV      { allocation },
      .mDSV      { allocation },
    };
  }

  void DX12TextureMgr::CreateRenderTargetColor( TextureHandle h,
                                                  PCom<ID3D12Resource> resource,
                                                  Errors& errors)
  {
    DX12Texture* texture = FindTexture( h );
    TAC_ASSERT( texture );

    const DX12DescriptorHeapAllocation allocation  { mCpuDescriptorHeapRTV->Allocate() };
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle { allocation.GetCPUHandle() };

    ID3D12Resource* pResource = resource.Get();
    mDevice->CreateRenderTargetView( pResource, nullptr, cpuHandle );

    *texture = DX12Texture
    {
      .mResource { resource },
      .mDesc     { resource->GetDesc() },
      .mState    { D3D12_RESOURCE_STATE_PRESENT }, // Render targets are created in present state
      .mRTV      { allocation },
    };
  }

  void DX12TextureMgr::UpdateTexture( TextureHandle h, UpdateTextureParams params )
  {
    DX12Texture& dynTex { mTextures[ h.GetIndex() ] };
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void DX12TextureMgr::DestroyTexture( TextureHandle h )
  {
    if( h.IsValid() )
      mTextures[ h.GetIndex() ] = {};
  }

  DX12Texture* DX12TextureMgr::FindTexture( TextureHandle h )
  {
    return h.IsValid() ? &mTextures[ h.GetIndex() ] : nullptr;
  }
} // namespace Tac::Render
