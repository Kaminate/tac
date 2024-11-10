#include "tac_dx12_texture_mgr.h" // self-inc
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{
  static bool IsPowerOfTwo( u32 n )
  {
    return ( n & ( n - 1 ) ) == 0;
  }

  static D3D12_RESOURCE_FLAGS GetResourceFlags( Binding binding )
  {
    D3D12_RESOURCE_FLAGS Flags{};

    if( Binding{} != ( binding & Binding::RenderTarget ) )
      Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if( Binding{} != ( binding & Binding::DepthStencil ) )
      Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    if( Binding{} != ( binding & Binding::UnorderedAccess ) )
      Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    return Flags;
  }

  static D3D12_RESOURCE_DESC  GetImageResourceDesc( CreateTextureParams params )
  {
    TAC_ASSERT( params.mMipCount );

    const Image& image{ params.mImage };
    const DXGI_FORMAT dxgiFmt{ TexFmtToDxgiFormat(  image.mFormat ) };

    const DXGI_SAMPLE_DESC SampleDesc
    {
      .Count   { 1 },
      .Quality {},
    };

    const D3D12_RESOURCE_FLAGS Flags{ GetResourceFlags( params.mBinding ) };

    const D3D12_RESOURCE_DESC textureResourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_TEXTURE2D },
      .Width            { ( UINT64 )image.mWidth  },
      .Height           { ( UINT64 )image.mHeight },
      .DepthOrArraySize { 1 },
      .MipLevels        { ( UINT16 )params.mMipCount },
      .Format           { dxgiFmt },
      .SampleDesc       { SampleDesc },
      .Flags            { Flags },
    };
    return textureResourceDesc;
  }

  static D3D12_RESOURCE_STATES GetBindingResourceState( Binding binding )
  {
    switch( binding )
    {
    case Binding::None:            return D3D12_RESOURCE_STATE_COMMON;
    case Binding::ShaderResource:  return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    case Binding::UnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case Binding::DepthStencil:    return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    default:
    {
      TAC_ASSERT_CRITICAL( "It is ambiguous what state you want to transition to."
                           "(Your binding has multiple values, but we can only "
                           "transition to 1 state)" );
      return D3D12_RESOURCE_STATE_COMMON;
    }
    }
  };

  // -----------------------------------------------------------------------------------------------

  struct DX12ClearValue
  {
    DX12ClearValue( CreateTextureParams params )
    {
      if( Binding{} != ( params.mBinding & Binding::DepthStencil ) )
        SetClearValue(
          D3D12_CLEAR_VALUE
          {
            .Format       { TexFmtToDxgiFormat( params.mImage.mFormat ) },
            .DepthStencil { .Depth { 1 } },
          } );

      if( Binding{} != ( params.mBinding & Binding::RenderTarget ) )
        SetClearValue(
          D3D12_CLEAR_VALUE
          {
            .Format { TexFmtToDxgiFormat( params.mImage.mFormat ) },
            .Color  { 0, 0, 0, 1 },
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

  // -----------------------------------------------------------------------------------------------

  struct UploadCalculator
  {
    UploadCalculator( UpdateTextureParams updateTextureParams )
    {
      const int nSubRsc{ updateTextureParams.mSrcSubresource.size() };
      const int imgW{ updateTextureParams.mSrcImage.mWidth };
      const int imgH{ updateTextureParams.mSrcImage.mHeight };
      const int texFmtSize{ GetTexFmtSize( updateTextureParams.mSrcImage.mFormat ) };

      int cumulativeSubRscOffset{};
      for( int iSubRsc{}; iSubRsc < nSubRsc; ++iSubRsc )
      {
        const int subRscW{ imgW >> iSubRsc };
        const int subRscH{ imgH >> iSubRsc };
        const int rowPitch{
          RoundUpToNearestMultiple( subRscW * texFmtSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT ) };
        const int subRscByteCount{ rowPitch * subRscH };

        mRowPitches[ iSubRsc ] = rowPitch;
        mCumSubRscOffsets[ iSubRsc ] = cumulativeSubRscOffset;
        cumulativeSubRscOffset +=
          RoundUpToNearestMultiple( subRscByteCount, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT );
      }

      mTotalSize = cumulativeSubRscOffset;
    }

    int GetCumulativeSubresourceOffset( int i ) const { return mCumSubRscOffsets[ i ]; }
    int GetPitch( int i ) const                       { return mRowPitches[i]; }
    int GetTotalSize() const                          { return mTotalSize; }

  private:
    int mRowPitches[ 20 ]{};
    int mCumSubRscOffsets[ 20 ]{};
    int mTotalSize{};
  };

  // -----------------------------------------------------------------------------------------------

  DX12TextureMgr::Bindings::Bindings( ID3D12Resource* pResource, Binding binding )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    ID3D12Device* mDevice{ renderer.mDevice };
    DX12DescriptorHeapMgr& heapMgr{ renderer.mDescriptorHeapMgr };

    Optional< DX12Descriptor > RTV;
    if( Binding{} != ( binding & Binding::RenderTarget ) )
    {
      DX12DescriptorHeap& heap{ heapMgr.mCPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_RTV ] };
      RTV = heap.Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { RTV->GetCPUHandle() };
      const D3D12_RENDER_TARGET_VIEW_DESC* pRTVDesc{};
      mDevice->CreateRenderTargetView( pResource, pRTVDesc, descDescriptor );
    }

    Optional< DX12Descriptor > DSV;
    if( Binding{} != ( binding & Binding::DepthStencil ) )
    {
      DX12DescriptorHeap& heap{ heapMgr.mCPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_DSV ] };
      DSV = heap.Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { DSV->GetCPUHandle() };
      const D3D12_DEPTH_STENCIL_VIEW_DESC* pDSVDesc{};
      mDevice->CreateDepthStencilView( pResource, pDSVDesc, descDescriptor );
    }

    Optional< DX12Descriptor > SRV;
    if( Binding{} != ( binding & Binding::ShaderResource ) )
    {
      DX12DescriptorHeap& heap{ heapMgr.mCPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ] };
      SRV = heap.Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { SRV->GetCPUHandle() };
      const D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc{};
      mDevice->CreateShaderResourceView( pResource, pSRVDesc, descDescriptor );
    }

    Optional< DX12Descriptor > UAV;
    if( Binding{} != ( binding & Binding::UnorderedAccess ) )
    {
      DX12DescriptorHeap& heap{ heapMgr.mCPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ] };
      UAV = heap.Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { UAV->GetCPUHandle() };
      const D3D12_UNORDERED_ACCESS_VIEW_DESC* pUAVDesc{};
      mDevice->CreateUnorderedAccessView( pResource,
                                          nullptr, // ID3D12Resource* pCounterResource
                                          pUAVDesc,
                                          descDescriptor );
    }

    mRTV = RTV;
    mDSV = DSV;
    mSRV = SRV;
    mUAV = UAV;
  }

  // -----------------------------------------------------------------------------------------------

  TextureHandle DX12TextureMgr::CreateTexture( CreateTextureParams params,
                                               Errors& errors )
  {
    DX12ContextManager* mContextManager{ &DX12Renderer::sRenderer.mContextManager};
    const bool hasImageBytes{ !params.mSubresources.empty() };
    const D3D12_RESOURCE_DESC textureResourceDesc{ GetImageResourceDesc( params ) };
    ID3D12Device* mDevice{ DX12Renderer::sRenderer.mDevice};

    const D3D12_HEAP_TYPE heapType{ params.mUsage == Usage::Staging
        ? D3D12_HEAP_TYPE_UPLOAD
        : D3D12_HEAP_TYPE_DEFAULT };

    const D3D12_RESOURCE_STATES intialResourceStates{
      [ & ]()
      {
        if( heapType == D3D12_HEAP_TYPE_UPLOAD )
          return D3D12_RESOURCE_STATE_GENERIC_READ;

        if( hasImageBytes )
          return D3D12_RESOURCE_STATE_COPY_DEST;

        return D3D12_RESOURCE_STATE_COMMON;
      }( ) };

    DX12Resource dx12Resource;

    if( params.mUsage == Usage::Default ||
        params.mUsage == Usage::Static )
    {
        const D3D12_HEAP_PROPERTIES heapProps{ .Type { heapType } };

        const DX12ClearValue clearValue( params );

        PCom< ID3D12Resource > resource;
        mDevice->CreateCommittedResource(
          &heapProps,
          D3D12_HEAP_FLAG_NONE,
          &textureResourceDesc,
          intialResourceStates,
          clearValue.mClearValuePointer,
          resource.iid(),
          resource.ppv() );

        dx12Resource = DX12Resource( resource, textureResourceDesc, intialResourceStates );
    }

    DX12Context* context{ mContextManager->GetContext( errors ) };
    DX12Context::Scope contextScope{ context };

    //ID3D12Resource* pResource { resource.Get() };

    if( hasImageBytes )
    {
      const UpdateTextureParams updateTextureParams
      {
        .mSrcImage            { params.mImage },
        .mSrcSubresource      { params.mSubresources },
        .mDstSubresourceIndex {},
        .mDstPos              {},
      };
      TAC_CALL_RET( {}, UploadTextureData( &dx12Resource,
                                           updateTextureParams,
                                           context,
                                           errors ) );
    }

    const Bindings bindings( dx12Resource, params.mBinding );

    ID3D12GraphicsCommandList* commandList{ context->GetCommandList() };

    if( const bool isSingleBinding{ IsPowerOfTwo( ( u32 )params.mBinding ) } )
    {
      DX12TransitionHelper transitionHelper;
      TransitionResource( &dx12Resource, params.mBinding, &transitionHelper );
      transitionHelper.ResourceBarrier( commandList );
    }

    // do we context->SetSynchronous() ?
    TAC_CALL_RET( {}, context->Execute( errors ) );

    //const D3D12_RESOURCE_DESC resourceDesc{ resource->GetDesc() };

    const TextureHandle h{ AllocTextureHandle() };
    const int iTexture{ h.GetIndex() };

    const DX12Name name
    {
      .mName          { params.mOptionalName },
      .mStackFrame    { params.mStackFrame },
      .mResourceType  { "Texture" },
      .mResourceIndex { iTexture },
    };
    DX12SetName( dx12Resource, name );


    mTextures[ iTexture ] = DX12Texture
    {
      .mResource          { dx12Resource },
      .mBinding           { params.mBinding },
      .mName              { params.mOptionalName },
      .mRTV               { bindings.mRTV },
      .mDSV               { bindings.mDSV },
      .mSRV               { bindings.mSRV },
      .mUAV               { bindings.mUAV },
    };
    return h;
  }

  void          DX12TextureMgr::UploadTextureData( DX12Resource* dstRsc,
                                                   UpdateTextureParams updateTextureParams,
                                                   DX12Context* context,
                                                   Errors& errors )
  {
    const CreateTextureParams::Subresources srcSubresources{ updateTextureParams.mSrcSubresource };
    const int nSubRscs{ srcSubresources.size() };
    if( !nSubRscs )
      return;

    const int texFmtSize{ GetTexFmtSize( updateTextureParams.mSrcImage.mFormat ) };

    ID3D12GraphicsCommandList* commandList{ context->GetCommandList() };

    // Resource barrier
    {
      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { dstRsc },
        .mStateAfter  { D3D12_RESOURCE_STATE_COPY_DEST },
      };
      DX12TransitionHelper transitionHelper;
      transitionHelper.Append( transitionParams );
      transitionHelper.ResourceBarrier( commandList );
    }

    const DXGI_FORMAT srcFmt{ TexFmtToDxgiFormat( updateTextureParams.mSrcImage.mFormat ) };

    const UploadCalculator uploadCalculator( updateTextureParams );

    const int uploadBytes{
      uploadCalculator.GetTotalSize() + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT };

    const DX12UploadAllocator::DynAlloc upload{
      context->mGPUUploadAllocator.Allocate( ( int )uploadBytes, errors ) };

    MemSet( upload.mCPUAddr, 0, upload.mByteCount );

    const int mip0Offset{ RoundUpToNearestMultiple( ( int )upload.mResourceOffest,
                                                    D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT ) };
    //void* mip0Ptr{ ( char* )upload.mUnoffsetCPUAddr 
    //  + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT  };

    for( int iSubRsc{}; iSubRsc < nSubRscs; ++iSubRsc )
    {
      const CreateTextureParams::Subresource subRsc{
        updateTextureParams.mSrcSubresource[ iSubRsc ] };

      const int uploadSubRscRowPitch{ uploadCalculator.GetPitch( iSubRsc ) };
      const int uploadSubRscOffset{
        mip0Offset +
        uploadCalculator.GetCumulativeSubresourceOffset( iSubRsc ) };

      const int srcW{ updateTextureParams.mSrcImage.mWidth >> iSubRsc };
      const int srcH{ updateTextureParams.mSrcImage.mHeight >> iSubRsc };

      for( int iRow{}; iRow < srcH; ++iRow )
      {
        char* src{ ( char* )subRsc.mBytes + subRsc.mPitch * iRow };
        char* dst{ ( char* )upload.mUnoffsetCPUAddr
          + uploadSubRscOffset
          + uploadSubRscRowPitch * iRow };

        if( iRow == 512 )
          ++asdf;


        MemCpy( dst, src, srcW * texFmtSize );
      }

      const D3D12_TEXTURE_COPY_LOCATION texCopyDst
      {
        .pResource        { dstRsc->Get() },
        .Type             { D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX },
        .SubresourceIndex { ( UINT )iSubRsc },
      };

      const D3D12_SUBRESOURCE_FOOTPRINT srcSubRscFootprint
      {
        .Format   { srcFmt },
        .Width    { ( UINT )srcW },
        .Height   { ( UINT )srcH },
        .Depth    { 1 },
        .RowPitch { ( UINT )uploadSubRscRowPitch },
      };

      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcPlacedSubRscFootprint
      {
        .Offset    { ( UINT64 )uploadSubRscOffset },
        .Footprint { srcSubRscFootprint },
      };

      const D3D12_TEXTURE_COPY_LOCATION texCopySrc
      {
        .pResource       { upload.mResource },
        .Type            { D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT },
        .PlacedFootprint { srcPlacedSubRscFootprint },
      };


      const UINT DstX{ ( UINT )updateTextureParams.mDstPos.x };
      const UINT DstY{ ( UINT )updateTextureParams.mDstPos.y };
      const UINT DstZ{};
      commandList->CopyTextureRegion( &texCopyDst, DstX, DstY, DstZ, &texCopySrc, nullptr );
    }
  }


  void          DX12TextureMgr::CreateRenderTargetColor( TextureHandle h,
                                                         PCom<ID3D12Resource> resource,
                                                         Errors& errors )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    ID3D12Device* device{ renderer.mDevice };
    DX12DescriptorHeapMgr& heapMgr{ renderer.mDescriptorHeapMgr };
    DX12DescriptorHeap& heap{ heapMgr.mCPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_RTV ] };
    DX12Texture* texture{ FindTexture( h ) };
    TAC_ASSERT( texture );

    const DX12Descriptor allocation{ heap.Allocate() };
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ allocation.GetCPUHandle() };

    ID3D12Resource* pResource{ resource.Get() };
    device->CreateRenderTargetView( pResource, nullptr, cpuHandle );

    // Render targets are created in present state
    const D3D12_RESOURCE_STATES state{ D3D12_RESOURCE_STATE_PRESENT };

    const DX12Resource dx12Resource( resource, resource->GetDesc(), state );

    *texture = DX12Texture
    {
      .mResource { dx12Resource },
      .mRTV      { allocation },
    };
  }

  void          DX12TextureMgr::UpdateTexture( TextureHandle h,
                                               UpdateTextureParams updateTextureParams,
                                               DX12Context* context,
                                               Errors& errors )
  {
    DX12Texture& texture{ mTextures[ h.GetIndex() ] };
    UploadTextureData( &texture.mResource,
                       updateTextureParams,
                       context,
                       errors );
  }

  void          DX12TextureMgr::DestroyTexture( TextureHandle h )
  {
    if( h.IsValid() )
    {
      FreeHandle( h );
      DX12Texture& texture{ mTextures[ h.GetIndex() ] };

      Optional< DX12Descriptor > optDescs[]
      {
        texture.mRTV,
        texture.mDSV,
        texture.mUAV,
        texture.mSRV,
      };

      for( Optional< DX12Descriptor > optDesc : optDescs )
      {
        if( optDesc.HasValue() )
        {
          DX12Descriptor desc{ optDesc.GetValue() };
          desc.mOwner->Free( desc );
        }
      }

      texture = {};
    }
  }

  DX12Texture*  DX12TextureMgr::FindTexture( TextureHandle h )
  {
    return h.IsValid() ? &mTextures[ h.GetIndex() ] : nullptr;
  }

  void          DX12TextureMgr::TransitionResource( DX12Resource* dx12Resource,
                                                    Binding binding,
                                                    DX12TransitionHelper* transitionHelper )
  {
    const D3D12_RESOURCE_STATES usageFromBinding{ GetBindingResourceState( binding ) };

    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { dx12Resource },
      .mStateAfter  { usageFromBinding },
    };
    transitionHelper->Append( transitionParams );
  }

  void          DX12TextureMgr::TransitionTexture( TextureHandle h,
                                                   DX12TransitionHelper* transitionHelper )
  {
    DX12Texture* texture{ FindTexture( h ) };
    TransitionResource( &texture->mResource,  texture->mBinding, transitionHelper );
  }

  void          DX12TextureMgr::SetName( TextureHandle h, StringView s )
  {
    if( DX12Texture * texture{ FindTexture( h ) } )
      DX12SetName( texture->mResource.Get(), s );
  }
} // namespace Tac::Render
