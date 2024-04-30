#include "tac_dx12_texture_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-win32/dx/dx12/tac_dx12_context_manager.h"
#include "tac-win32/dx/dxgi/tac_dxgi.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{

  static DXGI_FORMAT          GetImageDXGIFmt( const Image& image )
  {
    if( const TexFmt fmt{ image.mFormat2 }; fmt != TexFmt::kUnknown )
      return TexFmtToDxgiFormat( fmt );

    if( const Format& fmt{ image.mFormat };
        !( fmt.mElementCount == 0
        && fmt.mPerElementByteCount == 0
        && fmt.mPerElementDataType == GraphicsType::unknown ) )
      return GetDXGIFormatTexture( fmt );

    return DXGI_FORMAT_UNKNOWN;
  }

  static D3D12_RESOURCE_FLAGS GetResourceFlags( Binding binding )
  {
    D3D12_RESOURCE_FLAGS Flags{};
    if( Binding{} != ( binding & Binding::RenderTarget ) )
      Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if( Binding{} != ( binding & Binding::DepthStencil ) )
      Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    return Flags;
  }

  static D3D12_RESOURCE_DESC  GetImageResourceDesc( CreateTextureParams params )
  {
    const Image& image{ params.mImage };
    const DXGI_FORMAT dxgiFmt{ GetImageDXGIFmt( image ) };

    const DXGI_SAMPLE_DESC SampleDesc
    {
      .Count   { 1 },
      .Quality { 0 },
    };

    const D3D12_RESOURCE_FLAGS Flags{ GetResourceFlags( params.mBinding ) };

    const D3D12_RESOURCE_DESC textureResourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_TEXTURE2D },
      .Width            { ( UINT64 )image.mWidth  },
      .Height           { ( UINT64 )image.mWidth },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { dxgiFmt },
      .SampleDesc       { SampleDesc },
      .Flags            { Flags },
    };
    return textureResourceDesc;
  }

  struct DX12ClearValue
  {
    DX12ClearValue( CreateTextureParams params )
    {
      if( Binding{} != ( params.mBinding & Binding::DepthStencil ) )
        SetClearValue(
          D3D12_CLEAR_VALUE
          {
            .Format       { GetImageDXGIFmt( params.mImage ) },
            .DepthStencil {.Depth { 1.0 } },
          } );

      if( Binding{} != ( params.mBinding & Binding::RenderTarget ) )
        SetClearValue(
          D3D12_CLEAR_VALUE
          {
            .Format { GetImageDXGIFmt( params.mImage ) },
          } );
    }

    D3D12_CLEAR_VALUE* mClearValuePointer{};
  private:

    void SetClearValue( D3D12_CLEAR_VALUE clearValue )
    {
      mClearValue = clearValue;
      mClearValuePointer = &mClearValue;
    }

    D3D12_CLEAR_VALUE mClearValue;
    bool              mHasValue{};
  };

  void DX12TextureMgr::Init( Params params )
  {
    mDevice = params.mDevice;
    mCpuDescriptorHeapRTV = params.mCpuDescriptorHeapRTV;
    mCpuDescriptorHeapDSV = params.mCpuDescriptorHeapDSV;
    mContextManager = params.mContextManager;
    TAC_ASSERT( mDevice && mCpuDescriptorHeapRTV && mCpuDescriptorHeapDSV && mContextManager );
  }

  void DX12TextureMgr::CreateTexture( TextureHandle h,
                                      CreateTextureParams params,
                                      Errors& errors )
  {
    DX12Texture* texture{ FindTexture( h ) };
    TAC_ASSERT( texture );


    const bool hasImageBytes { params.mImageBytes || *params.mImageBytesCubemap };
    const D3D12_RESOURCE_DESC textureResourceDesc{ GetImageResourceDesc( params ) };

    params.mBinding & Binding::DepthStencil;

    D3D12_RESOURCE_STATES defaultHeapResourceStates{ D3D12_RESOURCE_STATE_COMMON };
    if( hasImageBytes )
      defaultHeapResourceStates = D3D12_RESOURCE_STATE_COPY_DEST;

    PCom< ID3D12Resource > defaultHeapResource;
    if( params.mAccess == Usage::Default )
    {
        const D3D12_HEAP_PROPERTIES defaultHeapProps{ .Type { D3D12_HEAP_TYPE_DEFAULT }, };

        const DX12ClearValue clearValue( params );

        mDevice->CreateCommittedResource(
          &defaultHeapProps,
          D3D12_HEAP_FLAG_NONE,
          &textureResourceDesc,
          defaultHeapResourceStates,
          clearValue.mClearValuePointer,
          defaultHeapResource.iid(),
          defaultHeapResource.ppv() );
    }

#if 0

    PCom< ID3D12Resource > uploadHeapResource;
    if( params.mAccess == Usage::Dynamic || hasImageBytes )
    {
      const D3D12_HEAP_PROPERTIES heapProps{ .Type { D3D12_HEAP_TYPE_DEFAULT }, };
      mDevice->CreateCommittedResource(
          &heapProps,
          D3D12_HEAP_FLAG_NONE,
          &textureResourceDesc,
          textureResourceStates,
          nullptr,
          defaultHeapResource.iid(),
          defaultHeapResource.ppv() );
    }

    if( params.mAccess == Usage::Default && hasImageBytes )
    {

      // Create upload heap

        UINT64 totalBytes;
        mDevice->GetCopyableFootprints( &textureResourceDesc,
                                        0,
                                        1,
                                        0,
                                        nullptr,
                                        nullptr,
                                        nullptr,
                                        &totalBytes );

        const DXGI_SAMPLE_DESC SampleDesc
        {
          .Count   { 1 },
          .Quality { 0 },
        };

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


        const D3D12_HEAP_PROPERTIES uploadHeapProps{ .Type { D3D12_HEAP_TYPE_UPLOAD }, };
        // Create the GPU upload buffer.
        TAC_CALL( mDevice->CreateCommittedResource(
          &uploadHeapProps,
          D3D12_HEAP_FLAG_NONE,
          &uploadBufferResourceDesc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          uploadHeapResource.iid(),
          uploadHeapResource.ppv() ) );
    }
    else
    {
      TAC_ASSERT_UNIMPLEMENTED;
    }


    // Copy data to the intermediate upload heap and then schedule a copy 
    // from the upload heap to the Texture2D.

    const D3D12_SUBRESOURCE_DATA textureData
    {
      .pData      { params.mImageBytes },
      .RowPitch   { Checkerboard::TexturePixelSize * Checkerboard::TextureWidth },
      .SlicePitch { Checkerboard::TexturePixelSize * Checkerboard::TextureWidth * Checkerboard::TextureHeight },
    };

    // --- update subresource begin ---

    const int nSubRes { 1 };
    Vector< D3D12_PLACED_SUBRESOURCE_FOOTPRINT > footprints( nSubRes ); // aka layouts?
    Vector< UINT64 > rowByteCounts( nSubRes );
    Vector< UINT > rowCounts( nSubRes );
    UINT64 requiredByteCount;
    mDevice->GetCopyableFootprints( &textureResourceDesc,
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
      TAC_DX12_CALL( uploadHeapResource->Map( (UINT)subresourceIndex, &readRange, &mappedData ) );

      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = footprints[ subresourceIndex ];
      const UINT rowCount { rowCounts[ subresourceIndex] };

      const D3D12_MEMCPY_DEST DestData 
      {
        .pData      { (char*)mappedData + layout.Offset },
        .RowPitch   { layout.Footprint.RowPitch },
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


      uploadHeapResource->Unmap( subresourceIndex, nullptr);
    }

    DX12Context* context{ mContextMgr->GetContext( errors ) };

    for( int iSubRes { 0 }; iSubRes < nSubRes; ++iSubRes )
    {
        const D3D12_TEXTURE_COPY_LOCATION Dst
        {
          .pResource        { (ID3D12Resource *)defaultHeapResource },
          .Type             { D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX },
          .SubresourceIndex { (UINT)iSubRes },
        };

        const D3D12_TEXTURE_COPY_LOCATION Src
        {
          .pResource       { (ID3D12Resource *)uploadHeapResource },
          .Type            { D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT },
          .PlacedFootprint { footprints[ iSubRes ] },
        };

        ID3D12GraphicsCommandList* cmdList = context->GetCommandList();
        cmdList->CopyTextureRegion( &Dst, 0, 0, 0, &Src, nullptr );
    }

    // --- update subresource end ---

    const TransitionParams transitionParams
    {
       .mResource     { ( ID3D12Resource* )m_texture },
       .mCurrentState { &m_textureResourceStates },
       .mTargetState  { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
    };

    TransitionResource( transitionParams );

    const D3D12_TEX2D_SRV Texture2D
    {
      .MipLevels { 1 },
    };

    // Describe and create a SRV for the texture.
    const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc 
    {
      .Format                  { textureResourceDesc.Format },
      .ViewDimension           { D3D12_SRV_DIMENSION_TEXTURE2D },
      .Shader4ComponentMapping { D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING },
      .Texture2D               { Texture2D },
    };

    context->SetSynchronous();
    context->Execute( errors );
    context->Retire();
#endif

    ID3D12Resource* pResource { defaultHeapResource.Get() };
    Bindings bindings{ CreateBindings( pResource, params.mBinding ) };

    *texture = DX12Texture
    {
      .mResource { defaultHeapResource },
      .mDesc     { defaultHeapResource->GetDesc() },
      .mState    { defaultHeapResourceStates },
      .mRTV      { bindings.mRTV },
      .mDSV      { bindings.mDSV },
    };
  }

  DX12TextureMgr::Bindings DX12TextureMgr::CreateBindings( ID3D12Resource* pResource,
                                                           Binding binding )
  {

    Optional< DX12DescriptorHeapAllocation > RTV;
    if( Binding{} != ( binding & Binding::RenderTarget ) )
    {
      RTV = mCpuDescriptorHeapRTV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { RTV->GetCPUHandle() };
      mDevice->CreateRenderTargetView( pResource, nullptr, descDescriptor );
    }

    Optional< DX12DescriptorHeapAllocation > DSV;
    if( Binding{} != ( binding & Binding::DepthStencil ) )
    {
      DSV = mCpuDescriptorHeapDSV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { DSV->GetCPUHandle() };
      mDevice->CreateDepthStencilView( pResource, nullptr, descDescriptor );
    }

    return Bindings
    {
      .mRTV{ RTV },
      .mDSV{ DSV },
    };
  }

  void DX12TextureMgr::CreateRenderTargetColor( TextureHandle h,
                                                PCom<ID3D12Resource> resource,
                                                Errors& errors )
  {
    DX12Texture* texture { FindTexture( h ) };
    TAC_ASSERT( texture );

    const DX12DescriptorHeapAllocation allocation  { mCpuDescriptorHeapRTV->Allocate() };
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle { allocation.GetCPUHandle() };

    ID3D12Resource* pResource { resource.Get() };
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
