#include "tac_dx12_buffer_mgr.h" // self-inc

#include "tac-dx/dx12/context/tac_dx12_context.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/tac_dx12_transition_helper.h"
#include "tac-dx/dxgi/tac_dxgi.h"

#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac::Render
{
  static constexpr bool kLogBufferLifetimes{ kIsDebugMode && true };

  static D3D12_RESOURCE_FLAGS GetResourceFlags( Binding binding )
  {
    D3D12_RESOURCE_FLAGS ResourceFlags{ D3D12_RESOURCE_FLAG_NONE };

    if( Binding{} == ( binding & Binding::ShaderResource ) )
      ResourceFlags &= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    if( Binding{} != ( binding & Binding::UnorderedAccess ) )
      ResourceFlags &= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    return ResourceFlags;
  }

  static DXGI_FORMAT GetSRVFormat( CreateBufferParams params )
  {
    if( params.mGpuBufferMode == GpuBufferMode::kStructured )
      return DXGIFormatFromTexFmt( params.mGpuBufferFmt );

    if( params.mGpuBufferMode == GpuBufferMode::kByteAddress )
      return DXGI_FORMAT_R32_TYPELESS;

    return DXGI_FORMAT_UNKNOWN;
  }

  static void LogBufferAux( StringView action, BufferHandle h, dynmc DX12Buffer* buffer )
  {
    if( !kLogBufferLifetimes )
      return;

    const auto name{ [ & ]() ->StringView {
      if( !buffer->mCreateName.empty() )
        return buffer->mCreateName;

      if( buffer->mResource )
        return DX12GetName( buffer->mResource );

      return "<unknown>";
    }( ) };


    const ShortFixedString renderFrameStr{ ShortFixedString::Concat(
      "(render frame: ", ToString( DX12Renderer::sRenderer.mRenderFrame ), ")" ) };

    const ShortFixedString handleStr{
      ShortFixedString::Concat( "(handle: ", ToString( h.GetIndex() ), ")" ) };

    LogApi::LogMessagePrint( action );
    LogApi::LogMessagePrint( " " );
    LogApi::LogMessagePrint( name );
    LogApi::LogMessagePrint( " " );
    LogApi::LogMessagePrint( handleStr );
    LogApi::LogMessagePrint( renderFrameStr );
    LogApi::LogMessagePrint( "\n" );
  }
  static void LogBufferDestruction( BufferHandle h, dynmc DX12Buffer* buffer )
  {
    LogBufferAux( "[destroying_buffer]", h, buffer );
  }
  static void LogBufferCreation( BufferHandle h, dynmc DX12Buffer* buffer )
  {
    LogBufferAux( "[creating_buffer]", h , buffer);
  }


  DX12BufferMgr::DescriptorBindings DX12BufferMgr::CreateBindings( ID3D12Resource* resource,
                                                                   CreateBufferParams params )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    DX12DescriptorHeapMgr& heapMgr{ renderer.mDescriptorHeapMgr };
    DX12DescriptorHeap& heap{ heapMgr.mCPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ] };

    ID3D12Device* mDevice{ renderer.mDevice };
    const Binding binding{ params.mBinding };

    if( Binding{} != ( binding & Binding::VertexBuffer ) )
    {
      TAC_ASSERT( params.mStride );
    }

    Optional< DX12Descriptor > srv;
    Optional< DX12Descriptor > uav;

    if( Binding{} != ( binding & Binding::ShaderResource ) )
    {
      const DX12Descriptor allocation{ heap.Allocate() };
      const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{ allocation.GetCPUHandle() };

      TAC_ASSERT( params.mGpuBufferMode != GpuBufferMode::kUndefined );

      const D3D12_BUFFER_SRV_FLAGS Flags{
        params.mGpuBufferMode == GpuBufferMode::kByteAddress
        ? D3D12_BUFFER_SRV_FLAG_RAW
        : D3D12_BUFFER_SRV_FLAG_NONE };

      const UINT NumElements{
        params.mGpuBufferMode == GpuBufferMode::kByteAddress
        ? ( UINT )params.mByteCount / 4 // due to DXGI_FORMAT_R32_TYPELESS
        : ( UINT )params.mByteCount / ( UINT )params.mStride
      };

      const UINT StructureByteStride{
        params.mGpuBufferMode == GpuBufferMode::kStructured
        ? ( UINT )params.mStride
        : ( UINT )0 };

      const D3D12_BUFFER_SRV Buffer
      {
        .FirstElement        {},
        .NumElements         { NumElements },
        .StructureByteStride { StructureByteStride },
        .Flags               { Flags },
      };

      const DXGI_FORMAT Format{ GetSRVFormat( params ) };

      const D3D12_SHADER_RESOURCE_VIEW_DESC Desc
      {
        .Format                  { Format },
        .ViewDimension           { D3D12_SRV_DIMENSION_BUFFER },
        .Shader4ComponentMapping { D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING },
        .Buffer                  { Buffer },
      };

      mDevice->CreateShaderResourceView( resource, &Desc, DestDescriptor );
      srv = allocation;
    }


    if( Binding{} != ( binding & Binding::UnorderedAccess ) )
    {
      const DX12Descriptor allocation{ heap.Allocate() };
      const D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor{ allocation.GetCPUHandle() };
      mDevice->CreateUnorderedAccessView( resource, nullptr, nullptr, DestDescriptor );
      srv = allocation;
    }

    return DescriptorBindings
    {
      .mSRV { srv },
      .mUAV { uav },
    };
  }

  BufferHandle DX12BufferMgr::CreateBuffer( CreateBufferParams params,
                                            Errors& errors )
  {
    const int byteCount { params.mByteCount };
    const StackFrame sf { params.mStackFrame };

    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    ID3D12Device* mDevice{ renderer.mDevice };
    DX12ContextManager* contextManager{ &renderer.mContextManager };

    if( params.mUsage == Usage::Dynamic )
    {
      const BufferHandle h{ AllocBufferHandle() };
      DX12Buffer& buffer{ mBuffers[ h.GetIndex() ] };
      buffer = DX12Buffer{ .mCreateParams { params }, };

      if( !params.mOptionalName.empty() )
      {
        buffer.mCreateName = params.mOptionalName;
        buffer.mCreateParams.mOptionalName = buffer.mCreateName;
      }

      LogBufferCreation( h, &mBuffers[ h.GetIndex() ] );

      return h;
    }

    D3D12_HEAP_TYPE heapType{ D3D12_HEAP_TYPE_DEFAULT };
    if( params.mUsage == Usage::Staging )
      heapType = D3D12_HEAP_TYPE_UPLOAD;

    const D3D12_HEAP_PROPERTIES HeapProps
    {
      .Type                 { heapType },
      .CPUPageProperty      { D3D12_CPU_PAGE_PROPERTY_UNKNOWN },
      .MemoryPoolPreference { D3D12_MEMORY_POOL_UNKNOWN },
      .CreationNodeMask     { 1 },
      .VisibleNodeMask      { 1 },
    };

    const DXGI_SAMPLE_DESC SampleDesc
    {
      .Count   { 1 },
      .Quality {},
    };

    const D3D12_RESOURCE_FLAGS ResourceFlags{ GetResourceFlags( params.mBinding ) };

    const D3D12_RESOURCE_DESC ResourceDesc
    {
      .Dimension        { D3D12_RESOURCE_DIMENSION_BUFFER },
      .Alignment        {},
      .Width            { ( UINT64 )byteCount },
      .Height           { 1 },
      .DepthOrArraySize { 1 },
      .MipLevels        { 1 },
      .Format           { DXGI_FORMAT_UNKNOWN },
      .SampleDesc       { SampleDesc },
      .Layout           { D3D12_TEXTURE_LAYOUT_ROW_MAJOR },
      .Flags            { ResourceFlags },
    };

    const D3D12_RESOURCE_STATES initialResourceStates{
      [ & ]()
      {
        if( heapType == D3D12_HEAP_TYPE_UPLOAD )
          return D3D12_RESOURCE_STATE_GENERIC_READ;

        // | Reason for commenting out:
        // | Warning: ID3D12Device::CreateCommittedResource2: Ignoring InitialState
        // | D3D12_RESOURCE_STATE_COPY_DEST. Buffers are effectively created in state
        // | D3D12_RESOURCE_STATE_COMMON.
        // v
        //if( params.mBytes )
        //  return D3D12_RESOURCE_STATE_COPY_DEST;

        return D3D12_RESOURCE_STATE_COMMON;
      }( ) };

    ID3D12Device* device{ mDevice };

    PCom< ID3D12Resource > committedResource;
    TAC_DX12_CALL_RET( {}, device->CreateCommittedResource(
      &HeapProps,
      D3D12_HEAP_FLAG_NONE,
      &ResourceDesc,
      initialResourceStates,
      nullptr,
      committedResource.iid(),
      committedResource.ppv() ) );

    DX12Resource buffer{ DX12Resource( committedResource, ResourceDesc, initialResourceStates ) };

    ID3D12Resource* resource{ buffer.Get() };


    void* mappedCPUAddr{};
    if( heapType == D3D12_HEAP_TYPE_UPLOAD )
        resource->Map( 0, nullptr, &mappedCPUAddr );

    DX12Context* context{ contextManager->GetContext( errors ) };
    IContext::Scope contextScope{ context };
    ID3D12GraphicsCommandList* commandList{ context->GetCommandList() };

    if( params.mBytes )
    {
      if( mappedCPUAddr )
      {
        MemCpy( mappedCPUAddr, params.mBytes, params.mByteCount );
      }
      else
      {

        const DX12TransitionHelper::Params transitionParams
        {
          .mResource    { &buffer },
          .mStateAfter  { D3D12_RESOURCE_STATE_COPY_DEST },
        };
        DX12TransitionHelper transitionHelper;
        transitionHelper.Append( transitionParams );
        transitionHelper.ResourceBarrier( commandList );

        DX12UploadAllocator::DynAlloc allocation{
          context->mGPUUploadAllocator.Allocate( byteCount, errors ) };

        MemCpy( allocation.mCPUAddr, params.mBytes, params.mByteCount );

        const UINT64 dstResourceOffset{};
        commandList->CopyBufferRegion( resource,
                                       dstResourceOffset,
                                       allocation.mResource,
                                       allocation.mResourceOffest,
                                       params.mByteCount );
      }
    }

    const DescriptorBindings descriptorBindings{ CreateBindings( resource, params ) };

    const BufferHandle h{ AllocBufferHandle() };
    const int i { h.GetIndex() };

    DX12NameHelper
    {
      .mName          { params.mOptionalName },
      .mStackFrame    { params.mStackFrame },
      .mHandle        { h },
    }.NameObject( resource );

    // Transition to the intended usage for context root signature binding
    TransitionBuffer( params.mBinding,
                      &buffer,
                      commandList );

    // do we context->SetSynchronous() ?
    TAC_CALL_RET( context->Execute( errors ) );

    const D3D12_GPU_VIRTUAL_ADDRESS gpuVritualAddress { buffer->GetGPUVirtualAddress() };

    mBuffers[ i ] = DX12Buffer
    {
      .mResource       { buffer },
      .mGPUVirtualAddr { gpuVritualAddress },
      .mSRV            { descriptorBindings.mSRV },
      .mUAV            { descriptorBindings.mUAV },
      .mMappedCPUAddr  { mappedCPUAddr },
      .mCreateParams   { params },
    };

    LogBufferCreation( h, &mBuffers[ i ] );

    return h;
  }

  DX12Buffer* DX12BufferMgr::FindBuffer( BufferHandle h )
  {
    return h.IsValid() ? &mBuffers[ h.GetIndex() ] : nullptr;
  }

  void DX12BufferMgr::TransitionBuffer( Binding binding,
                                        DX12Resource* resource,
                                        ID3D12GraphicsCommandList* commandList )
  {
      D3D12_RESOURCE_STATES usageFromBinding{ D3D12_RESOURCE_STATE_COMMON };

      if( Binding{} != ( binding & Binding::ShaderResource ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;

      if( Binding{} != ( binding & Binding::RenderTarget ) )
      {
        // (new) How would this be hit? This is the buffer mgr, not the texture mgr
        TAC_ASSERT_INVALID_CODE_PATH;

        usageFromBinding |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        // D3D12_RESOURCE_STATE_PRESENT ?
      }

      if( Binding{} != ( binding & Binding::DepthStencil ) )
      {
        // (new) How would this be hit? This is the buffer mgr, not the texture mgr
        TAC_ASSERT_INVALID_CODE_PATH;


        usageFromBinding |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        // D3D12_RESOURCE_STATE_DEPTH_READ ?
      }

      if( Binding{} != ( binding & Binding::UnorderedAccess ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

      if( Binding{} != ( binding & Binding::ConstantBuffer ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

      if( Binding{} != ( binding & Binding::VertexBuffer ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
      
      if( Binding{} != ( binding & Binding::IndexBuffer ) )
        usageFromBinding |= D3D12_RESOURCE_STATE_INDEX_BUFFER;


      const DX12TransitionHelper::Params transitionParams
      {
        .mResource    { resource },
        .mStateAfter  { usageFromBinding },
      };

      DX12TransitionHelper transitionHelper;
      transitionHelper.Append( transitionParams );
      transitionHelper.ResourceBarrier( commandList );
  }

  void DX12BufferMgr::UpdateBuffer( BufferHandle h,
                                    Span< const UpdateBufferParams > paramSpan,
                                    DX12Context* context,
                                    Errors& errors )
  {
    DX12Buffer& buffer{ mBuffers[ h.GetIndex() ] };
    ID3D12Resource* resource{ buffer.mResource.Get() };

    if( buffer.mCreateParams.mUsage == Usage::Default )
    {
      TAC_ASSERT( !buffer.mMappedCPUAddr );
      TAC_DX12_CALL( buffer.mResource->Map( 0, nullptr, &buffer.mMappedCPUAddr ) );
    }

    if( buffer.mCreateParams.mUsage == Usage::Dynamic )
    {
      const int byteCount{ RoundUpToNearestMultiple(
        buffer.mCreateParams.mByteCount,
        D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT ) };

      //ID3D12GraphicsCommandList* commandList{ context->GetCommandList() };
      DX12UploadAllocator* uploadAllocator{ &context->mGPUUploadAllocator };

      const DX12UploadAllocator::DynAlloc allocation{
        uploadAllocator->Allocate( byteCount, errors ) };

      buffer.mMappedCPUAddr = allocation.mCPUAddr;
      buffer.mGPUVirtualAddr = allocation.mGPUAddr;
      buffer.mResource.SetState( *allocation.mResourceState ); // ???

      // uhh so like if the allocation could be used for other things, then
      // i dont really want to use buffer.mCreateParams.mOptionalName/buffer.mCreateName;

      // Do i even want descriptor bindings? isnt that only useful if the buffer is sent through
      // a descriptor table? but i think dynamic buffers may be used through root parameters only
    }

    for( const UpdateBufferParams& params : paramSpan )
    {
      TAC_ASSERT( buffer.mMappedCPUAddr );
      if( !params.mSrcBytes || params.mSrcByteCount <= 0 )
        continue;

      char* dstBytes{ ( char* )buffer.mMappedCPUAddr + params.mDstByteOffset };
      MemCpy( dstBytes, params.mSrcBytes, params.mSrcByteCount );
    }

    if( buffer.mCreateParams.mUsage == Usage::Default  )
    {
      buffer.mResource->Unmap( 0, nullptr );
    }
  }

  void DX12BufferMgr::DestroyBuffer( BufferHandle h )
  {
    if( h.IsValid() )
    {
      LogBufferDestruction( h, &mBuffers[ h.GetIndex() ] );
      FreeHandle( h );
      mBuffers[ h.GetIndex() ] = {};
    }
  }
} // namespace Tac::Render
