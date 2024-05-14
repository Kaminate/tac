#include "tac_dx12_texture_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-win32/dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-win32/dx/dx12/tac_dx12_transition_helper.h"
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
      .Height           { ( UINT64 )image.mHeight },
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
            .DepthStencil { .Depth { 1 } },
          } );

      if( Binding{} != ( params.mBinding & Binding::RenderTarget ) )
        SetClearValue(
          D3D12_CLEAR_VALUE
          {
            .Format { GetImageDXGIFmt( params.mImage ) },
            .Color  {},
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
    mCpuDescriptorHeapCBV_SRV_UAV = params.mCpuDescriptorHeapCBV_SRV_UAV;
    mContextManager = params.mContextManager;
    TAC_ASSERT( mDevice );
    TAC_ASSERT( mCpuDescriptorHeapRTV );
    TAC_ASSERT( mCpuDescriptorHeapDSV );
    TAC_ASSERT( mCpuDescriptorHeapCBV_SRV_UAV );
    TAC_ASSERT( mContextManager );
  }

  void DX12TextureMgr::CreateTexture( TextureHandle h,
                                      CreateTextureParams params,
                                      Errors& errors )
  {
    DX12Texture* texture{ FindTexture( h ) };
    TAC_ASSERT( texture );

    const bool hasImageBytes { params.mImageBytes || *params.mImageBytesCubemap };
    const D3D12_RESOURCE_DESC textureResourceDesc{ GetImageResourceDesc( params ) };

    const D3D12_HEAP_TYPE heapType{ params.mUsage == Usage::Staging
        ? D3D12_HEAP_TYPE_UPLOAD
        : D3D12_HEAP_TYPE_DEFAULT };

    D3D12_RESOURCE_STATES resourceStates{
      [ & ]()
      {
        if( heapType == D3D12_HEAP_TYPE_UPLOAD )
          return D3D12_RESOURCE_STATE_GENERIC_READ;

        if( hasImageBytes )
          return D3D12_RESOURCE_STATE_COPY_DEST;

        return D3D12_RESOURCE_STATE_COMMON;
      }( ) };


    PCom< ID3D12Resource > resource;
    if( params.mUsage == Usage::Default )
    {
        const D3D12_HEAP_PROPERTIES heapProps{ .Type { heapType } };

        const DX12ClearValue clearValue( params );

        mDevice->CreateCommittedResource(
          &heapProps,
          D3D12_HEAP_FLAG_NONE,
          &textureResourceDesc,
          resourceStates,
          clearValue.mClearValuePointer,
          resource.iid(),
          resource.ppv() );

        ID3D12Resource* pResource{ resource.Get() };
        const DX12Name name
        {
          .mName          { params.mOptionalName },
          .mStackFrame    { params.mStackFrame },
          .mResourceType  { "Texture" },
          .mResourceIndex { h.GetIndex() },
        };
        DX12SetName( pResource, name );
    }


    ID3D12Resource* pResource { resource.Get() };

    if( hasImageBytes )
    {
      TAC_CALL( InitializeResourceData( pResource, heapType, params, errors ) );
    }

    Bindings bindings{ CreateBindings( pResource, params.mBinding ) };

    {
      D3D12_RESOURCE_STATES usageFromBinding{ D3D12_RESOURCE_STATE_COMMON };
      if( Binding{} != ( params.mBinding & Binding::ShaderResource ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
      DX12Context::Scope contextScope{ mContextManager->GetContext( errors ) };
      DX12Context* context{ ( DX12Context* )contextScope.GetContext() };
      ID3D12GraphicsCommandList* commandList { context->GetCommandList() };

      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { pResource },
        .mStateBefore { &resourceStates },
        .mStateAfter  { usageFromBinding },
      };
      DX12TransitionHelper transitionHelper;
      transitionHelper.Append( transitionParams );
      transitionHelper.ResourceBarrier( commandList );
      // do we context->SetSynchronous() ?
      TAC_CALL( context->Execute( errors ) );
    }

    const D3D12_RESOURCE_DESC resourceDesc{ resource->GetDesc() };
    *texture = DX12Texture
    {
      .mResource { resource },
      .mDesc     { resourceDesc },
      .mState    { resourceStates },
      .mRTV      { bindings.mRTV },
      .mDSV      { bindings.mDSV },
      .mSRV      { bindings.mSRV },
    };
  }

  void DX12TextureMgr::InitializeResourceData( ID3D12Resource* dstRsc,
                                               D3D12_HEAP_TYPE dstType,
                                               CreateTextureParams params,
                                               Errors& errors )
  {
    if( !params.mImageBytes  && !*params.mImageBytesCubemap )
      return;

    const D3D12_RESOURCE_DESC dstRscDesc{ dstRsc->GetDesc() };
    const D3D12_RESOURCE_DESC resourceDesc{ dstRsc->GetDesc() };

    ID3D12Resource* upload{ dstType == D3D12_HEAP_TYPE_UPLOAD ? dstRsc : nullptr };

    if( !upload )
    {
      int bytesPerPixel_Fmt = params.mImage.mFormat.CalculateTotalByteCount();
       params.mImage.mFormat2;
      //int bytesPerPixel_Fmt2 = params.mImage.mFormat2;

       int rowPitch = RoundUpToNearestMultiple( params.mImage.mWidth * bytesPerPixel_Fmt,
                                                D3D12_TEXTURE_DATA_PITCH_ALIGNMENT );

      // Create upload heap
       int totalBytesExpected = rowPitch * params.mImage.mHeight;

       UINT64 totalBytes;
       mDevice->GetCopyableFootprints( &dstRscDesc,
                                       0, // first subresource
                                       1, // num subresources
                                       0, // base offset
                                       nullptr, // layouts
                                       nullptr, // num rows
                                       nullptr, // row size in bytes
                                       &totalBytes );


       ++asdf;
    }

#if 0



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
  }

  DX12TextureMgr::Bindings DX12TextureMgr::CreateBindings( ID3D12Resource* pResource,
                                                           Binding binding )
  {

    Optional< DX12Descriptor > RTV;
    if( Binding{} != ( binding & Binding::RenderTarget ) )
    {
      RTV = mCpuDescriptorHeapRTV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { RTV->GetCPUHandle() };
      mDevice->CreateRenderTargetView( pResource, nullptr, descDescriptor );
    }

    Optional< DX12Descriptor > DSV;
    if( Binding{} != ( binding & Binding::DepthStencil ) )
    {
      DSV = mCpuDescriptorHeapDSV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { DSV->GetCPUHandle() };
      mDevice->CreateDepthStencilView( pResource, nullptr, descDescriptor );
    }

    Optional< DX12Descriptor > SRV;
    if( Binding{} != ( binding & Binding::ShaderResource ) )
    {
      SRV = mCpuDescriptorHeapCBV_SRV_UAV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { SRV->GetCPUHandle() };
      mDevice->CreateShaderResourceView( pResource, nullptr, descDescriptor );
    }

    return Bindings
    {
      .mRTV{ RTV },
      .mDSV{ DSV },
      .mSRV{ SRV },
    };
  }

  void DX12TextureMgr::CreateRenderTargetColor( TextureHandle h,
                                                PCom<ID3D12Resource> resource,
                                                Errors& errors )
  {
    DX12Texture* texture { FindTexture( h ) };
    TAC_ASSERT( texture );

    const DX12Descriptor allocation  { mCpuDescriptorHeapRTV->Allocate() };
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

  void DX12TextureMgr::UpdateTexture( TextureHandle h,
                                      UpdateTextureParams params,
                                      DX12Context* context,
                                      Errors& errors )
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
