#include "tac_dx12_texture_mgr.h" // self-inc
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"

#if !TAC_DELETE_ME()
#include "tac-std-lib/os/tac_os.h"
#endif

namespace Tac::Render
{
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
    TAC_ASSERT( params.mMipCount );

    const Image& image{ params.mImage };
    const DXGI_FORMAT dxgiFmt{ TexFmtToDxgiFormat(  image.mFormat ) };

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
      .MipLevels        { ( UINT16 )params.mMipCount },
      .Format           { dxgiFmt },
      .SampleDesc       { SampleDesc },
      .Flags            { Flags },
    };
    return textureResourceDesc;
  }

  static D3D12_RESOURCE_STATES GetBindingResourceState( Binding binding )
  {
    if( Binding{} != ( binding & Binding::ShaderResource ) )
      return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

    return D3D12_RESOURCE_STATE_COMMON;
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

  // -----------------------------------------------------------------------------------------------

  struct UploadPitchCalculator
  {
    UploadPitchCalculator( UpdateTextureParams updateTextureParams )
    {
      const CreateTextureParams::Subresources srcSubresources{ updateTextureParams.mSrcSubresource };
      const int nSubRscs{ srcSubresources.size() };
      const int texFmtSize{ GetTexFmtSize( updateTextureParams.mSrcImage.mFormat ) };
      for( int iSubRsc{}; iSubRsc < nSubRscs; ++iSubRsc )
      {
        const int srcW{ updateTextureParams.mSrcImage.mWidth >> iSubRsc };
        const int srcH{ updateTextureParams.mSrcImage.mHeight >> iSubRsc };
        const int uploadSubRscPitch{
          RoundUpToNearestMultiple( srcW * texFmtSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT ) };

        mUploadPitches[ iSubRsc ] = uploadSubRscPitch;
      }
    }

    int GetPitch( int i ) const { return mUploadPitches[i]; }
    int GetTotalSize() const 
    {
      int total{};
      for( int pitch : mUploadPitches )
        total += pitch;
      return total;
    }

  private:
    int mUploadPitches[ 20 ]{};
  };

  // -----------------------------------------------------------------------------------------------

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

    const bool hasImageBytes{ !params.mSubresources.empty() };
    const D3D12_RESOURCE_DESC textureResourceDesc{ GetImageResourceDesc( params ) };

    const D3D12_HEAP_TYPE heapType{ params.mUsage == Usage::Staging
        ? D3D12_HEAP_TYPE_UPLOAD
        : D3D12_HEAP_TYPE_DEFAULT };

    dynmc D3D12_RESOURCE_STATES resourceStates{
      [ & ]()
      {
        if( heapType == D3D12_HEAP_TYPE_UPLOAD )
          return D3D12_RESOURCE_STATE_GENERIC_READ;

        if( hasImageBytes )
          return D3D12_RESOURCE_STATE_COPY_DEST;

        return D3D12_RESOURCE_STATE_COMMON;
      }( ) };


    PCom< ID3D12Resource > resource;
    if( params.mUsage == Usage::Default ||
        params.mUsage == Usage::Static )
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

    DX12Context* context{ mContextManager->GetContext( errors ) };
    DX12Context::Scope contextScope{ context };

    ID3D12Resource* pResource { resource.Get() };

    if( hasImageBytes )
    {
      const UpdateTextureParams updateTextureParams
      {
        .mSrcImage            { params.mImage },
        .mSrcSubresource      { params.mSubresources },
        .mDstSubresourceIndex {},
        .mDstPos              {},
      };
      TAC_CALL( UploadTextureData(
        pResource,
        &resourceStates,
        updateTextureParams,
        context,
        errors ) );
    }

    const Bindings bindings{ CreateBindings( pResource, params.mBinding ) };

    ID3D12GraphicsCommandList* commandList{ context->GetCommandList() };

    DX12TransitionHelper transitionHelper;
    TransitionResource( pResource, &resourceStates, params.mBinding, &transitionHelper );
    transitionHelper.ResourceBarrier( commandList );

    // do we context->SetSynchronous() ?
    TAC_CALL( context->Execute( errors ) );

    const D3D12_RESOURCE_DESC resourceDesc{ resource->GetDesc() };
    *texture = DX12Texture
    {
      .mResource          { resource },
      .mDesc              { resourceDesc },
      .mState             { resourceStates },
      .mBinding           { params.mBinding },
      .mName              { params.mOptionalName },
      .mRTV               { bindings.mRTV },
      .mDSV               { bindings.mDSV },
      .mSRV               { bindings.mSRV },
    };
  }

  void DX12TextureMgr::UploadTextureData( ID3D12Resource* dstRsc,
                                          D3D12_RESOURCE_STATES* dstRscStates,
                                          UpdateTextureParams updateTextureParams,
                                          DX12Context* context,
                                          Errors& errors )
  {

    const CreateTextureParams::Subresources srcSubresources{ updateTextureParams.mSrcSubresource };
    const int nSubRscs{ srcSubresources.size() };
    if( !nSubRscs )
      return;

    const int texFmtSize{ GetTexFmtSize( updateTextureParams.mSrcImage.mFormat ) };

    ID3D12GraphicsCommandList* commandList { context->GetCommandList() };
    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { dstRsc },
      .mStateBefore { dstRscStates },
      .mStateAfter  { D3D12_RESOURCE_STATE_COPY_DEST },
    };
    DX12TransitionHelper transitionHelper;
    transitionHelper.Append( transitionParams );
    transitionHelper.ResourceBarrier( commandList );

    const D3D12_RESOURCE_DESC dstRscDesc{ dstRsc->GetDesc() };

    const DXGI_FORMAT srcFmt{ TexFmtToDxgiFormat( updateTextureParams.mSrcImage.mFormat ) };

    const UploadPitchCalculator uploadPitches( updateTextureParams );

    //int uploadPitches[ 20 ]{};
    //for( int iSubRsc{}; iSubRsc < nSubRscs; ++iSubRsc )
    //{
    //  //CreateTextureParams::Subresource subRsc{ updateTextureParams.mSrcSubresource[ iSubRsc ] };
    //  const int srcW{ updateTextureParams.mSrcImage.mWidth >> iSubRsc };
    //  const int srcH{ updateTextureParams.mSrcImage.mHeight >> iSubRsc };
    //  const int uploadSubRscPitch{
    //    RoundUpToNearestMultiple( srcW * texFmtSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT ) };

    //  uploadPitches[ iSubRsc ] = uploadSubRscPitch;
    //}

    const int uploadBytes{ uploadPitches.GetTotalSize() };

    //CreateTextureParams::Subresources mSrcSubresource;
    //D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

    // total number of bytes for the upload buffer to hold every subresource
    //UINT64 totalBytes;
    //Vector< D3D12_PLACED_SUBRESOURCE_FOOTPRINT > dstPlacedSubRscFootprints( nSubRscs );
    //Vector< UINT > dstRowCounts( nSubRscs );
    //Vector< UINT64 > dstRowByteCounts( nSubRscs );
    //mDevice->GetCopyableFootprints( &dstRscDesc,
    //                                0, // first subresource
    //                                nSubRscs,
    //                                0, // base offset
    //                                nullptr, //dstPlacedSubRscFootprints.data(),
    //                                nullptr, //dstRowCounts.data(),
    //                                nullptr, //dstRowByteCounts.data(),
    //                                &totalBytes );

    DX12UploadAllocator::DynAlloc upload{
      context->mGPUUploadAllocator.Allocate( ( int )uploadBytes, errors ) };

    ID3D12Resource* uploadRsc{ upload.mResource };

    //upload.m

    char* runningUploadData{ ( char* )upload.mCPUAddr };
    int runningUploadOffset{ ( int )upload.mResourceOffest };

    // for each subresource
    for( int iSubRsc{}; iSubRsc < nSubRscs; ++iSubRsc )
    {
      const CreateTextureParams::Subresource subRsc{
        updateTextureParams.mSrcSubresource[ iSubRsc ] };

      const int uploadSubRscPitch{ uploadPitches.GetPitch( iSubRsc ) };
      const int uploadSubRscOffset{ runningUploadOffset };
      void* uploadSubRscData{ runningUploadData };

      const int srcW{ updateTextureParams.mSrcImage.mWidth >> iSubRsc };
      const int srcH{ updateTextureParams.mSrcImage.mHeight >> iSubRsc };
      //const int uploadSubRscPitch{
      //  RoundUpToNearestMultiple( srcW * texFmtSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT ) };

      for( int iRow{}; iRow < srcH; ++iRow )
      {
        MemCpy( ( char* )uploadSubRscData + uploadSubRscPitch * iRow, // runningUploadData,
                ( char* )subRsc.mBytes + subRsc.mPitch * iRow,
                srcW * texFmtSize );

        //runningUploadData += uploadSubRscPitch;
        runningUploadOffset += uploadSubRscPitch;
      }


      const D3D12_TEXTURE_COPY_LOCATION texCopyDst
      {
        .pResource        { dstRsc },
        .Type             { D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX },
        .SubresourceIndex { ( UINT )iSubRsc },
      };

      const D3D12_SUBRESOURCE_FOOTPRINT srcSubRscFootprint
      {
        .Format   { srcFmt },
        .Width    { ( UINT )srcW },
        .Height   { ( UINT )srcH },
        .Depth    { 1 },
        .RowPitch { ( UINT )uploadSubRscPitch },
      };

      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcPlacedSubRscFootprint
      {
        .Offset    { ( UINT64 )uploadSubRscOffset },
        .Footprint { srcSubRscFootprint },
      };

      const D3D12_TEXTURE_COPY_LOCATION texCopySrc
      {
        .pResource       { ( ID3D12Resource* )uploadRsc },
        .Type            { D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT },
        .PlacedFootprint { srcPlacedSubRscFootprint },
      };


      const UINT DstX{ ( UINT )updateTextureParams.mDstPos.x };
      const UINT DstY{ ( UINT )updateTextureParams.mDstPos.y };
      const UINT DstZ{ 0 };
      commandList->CopyTextureRegion( &texCopyDst, DstX, DstY, DstZ, &texCopySrc, nullptr );
    }
  }

  DX12TextureMgr::Bindings DX12TextureMgr::CreateBindings( ID3D12Resource* pResource,
                                                           Binding binding )
  {

    Optional< DX12Descriptor > RTV;
    if( Binding{} != ( binding & Binding::RenderTarget ) )
    {
      RTV = mCpuDescriptorHeapRTV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { RTV->GetCPUHandle() };
      const D3D12_RENDER_TARGET_VIEW_DESC* pRTVDesc{};
      mDevice->CreateRenderTargetView( pResource, pRTVDesc, descDescriptor );
    }

    Optional< DX12Descriptor > DSV;
    if( Binding{} != ( binding & Binding::DepthStencil ) )
    {
      DSV = mCpuDescriptorHeapDSV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { DSV->GetCPUHandle() };
      const D3D12_DEPTH_STENCIL_VIEW_DESC* pDSVDesc{};
      mDevice->CreateDepthStencilView( pResource, pDSVDesc, descDescriptor );
    }

    Optional< DX12Descriptor > SRV;
    if( Binding{} != ( binding & Binding::ShaderResource ) )
    {
      SRV = mCpuDescriptorHeapCBV_SRV_UAV->Allocate();
      const D3D12_CPU_DESCRIPTOR_HANDLE descDescriptor { SRV->GetCPUHandle() };
      const D3D12_SHADER_RESOURCE_VIEW_DESC* pSRVDesc{};
      mDevice->CreateShaderResourceView( pResource, pSRVDesc, descDescriptor );
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
                                      UpdateTextureParams updateTextureParams,
                                      DX12Context* context,
                                      Errors& errors )
  {
    DX12Texture& texture{ mTextures[ h.GetIndex() ] };
    ID3D12Resource* resource{ texture.mResource.Get() };
    UploadTextureData( resource,
                       &texture.mState,
                       updateTextureParams,
                       context,
                       errors );
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

  void DX12TextureMgr::TransitionResource( ID3D12Resource* resource,
                                           D3D12_RESOURCE_STATES* resourceState,
                                           Binding binding,
                                           DX12TransitionHelper* transitionHelper )
  {
    const D3D12_RESOURCE_STATES usageFromBinding{ GetBindingResourceState( binding ) };

    const DX12TransitionHelper::Params transitionParams
    {
      .mResource    { resource },
      .mStateBefore { resourceState },
      .mStateAfter  { usageFromBinding },
    };
    transitionHelper->Append( transitionParams );
  }

  void DX12TextureMgr::TransitionTexture( TextureHandle h,
                                          DX12TransitionHelper* transitionHelper )
  {
    DX12Texture* texture{ FindTexture( h ) };
    ID3D12Resource* resource{ texture->mResource.Get() };
    TransitionResource( resource, &texture->mState, texture->mBinding, transitionHelper );
  }
} // namespace Tac::Render