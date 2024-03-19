#include "tac_renderer_dx12_ver3.h" // self-inc

//#include "tac-rhi/render3/tac_render_api.h"

//#include "tac-std-lib/os/tac_os.h"
//#include "tac-rhi/render/tac_render.h"
//#include "tac-rhi/render/tac_render.h"
//#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dxgi/tac_dxgi.h"

#pragma comment( lib, "d3d12.lib" ) // D3D12...

namespace Tac::Render
{

  /*
  // -----------------------------------------------------------------------------------------------

  // DX12CommandList

  void DX12CommandList::Draw()
  {
    ID3D12GraphicsCommandList* cmdList = mContext.GetCommandList();
    const UINT vtxCountPerInstance = 0;
    const UINT instanceCount = 0;
    const UINT startVertexLocation = 0;
    const UINT startIndexLocation = 0;
    cmdList->DrawInstanced( vtxCountPerInstance,
                            instanceCount,
                            startVertexLocation,
                            startIndexLocation );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12DynBuf

  void DX12DynBuf::SetName( StringView name )
  {
    DX12SetName( mResource, name );
  }

  // -----------------------------------------------------------------------------------------------

  // DX12Backend
  */

  void DX12Backend::Init( Errors& errors )
  {
    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( mDebugLayer.Init( errors ) );

    TAC_CALL( mDevice.Init( mDebugLayer, errors ) );

    ID3D12Device* device = mDevice.GetID3D12Device();

    TAC_CALL( mInfoQueue.Init( mDebugLayer, device, errors ) );

    //const int maxGPUFrameCount = RenderApi::GetMaxGPUFrameCount();
    /*
    const int maxGPUFrameCount = Render::GetMaxGPUFrameCount();
    TAC_ASSERT( maxGPUFrameCount );
    mFenceValues.resize( maxGPUFrameCount );

    TAC_CALL( DXGIInit( errors ) );

    TAC_CALL( debugLayer.Init( errors ) );

    TAC_CALL( mDevice.Init( debugLayer, errors ) );
    ID3D12Device* device = mDevice.GetID3D12Device();

    TAC_CALL( infoQueue.Init( debugLayer, device, errors ) );

    TAC_CALL( mCommandQueue.Create( device, errors ) );

    TAC_CALL( mRTVDescriptorHeap.InitRTV( 100, device, errors ) );
    TAC_CALL( mSRVDescriptorHeap.InitSRV( 100, device, errors ) );
    TAC_CALL( mSamplerDescriptorHeap.InitSampler( 100, device, errors ) );

    mCommandAllocatorPool.Init( device, &mCommandQueue );
    mContextManager.Init( &mCommandAllocatorPool,
                          &mCommandQueue,
                          &mUploadPageManager,
                          device );

    mUploadPageManager.Init( device, &mCommandQueue );

    mSamplers.Init( device, &mSamplerDescriptorHeap );
    */
  }
  /*

  SmartPtr< ICommandList > DX12Backend::GetCommandList( ContextHandle handle, Errors& errors )
  {
    const int i = handle.GetHandleIndex();
    if( !( i < mContexts.size() ) )
    {
      mContexts.resize( i + 1 );
    }

    DX12Context context = mContextManager.GetContextNoScope( errors );
    mContexts[ i ] = context;

    DX12CommandList* dx12CmdList = TAC_NEW DX12CommandList;
    dx12CmdList->mContext = context;

    return SmartPtr< ICommandList >{ dx12CmdList };
  }

  void DX12Backend::CreateDynamicBuffer2( const DynBufCreateParams& params, Errors& errors )
  {
    const D3D12_HEAP_PROPERTIES HeapProps
    {
      .Type = D3D12_HEAP_TYPE_UPLOAD,
      .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
      .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
      .CreationNodeMask = 1,
      .VisibleNodeMask = 1,
    };

    const D3D12_RESOURCE_DESC ResourceDesc
    {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Alignment = 0,
      .Width = ( UINT64 )params.mByteCount,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = DXGI_SAMPLE_DESC
      {
        .Count = 1,
        .Quality = 0
      },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    const D3D12_RESOURCE_STATES DefaultUsage{ D3D12_RESOURCE_STATE_GENERIC_READ };

    ID3D12Device* device = mDevice.GetID3D12Device();

    PCom< ID3D12Resource > buffer;
    TAC_DX12_CALL( device->CreateCommittedResource(
      &HeapProps,
      D3D12_HEAP_FLAG_NONE,
      &ResourceDesc,
      DefaultUsage,
      nullptr,
      buffer.iid(),
      buffer.ppv() ) );

    DX12SetName( buffer, params.mStackFrame );

    void* cpuAddr;

    TAC_DX12_CALL( buffer->Map(
      0, // subrsc idx
      nullptr, // nullptr indicates the whole subrsc may be read by cpu
      &cpuAddr ) );

    const int i = params.mHandle.GetHandleIndex();
    const int n = mDynBufs.size();
    if( !( i < n ) )
      mDynBufs.resize( i + 1 );

    mDynBufs[ i ] = DX12DynBuf
    {
      .mResource = buffer,
      .mMappedCPUAddr = cpuAddr,
    };
  }

  void DX12Backend::UpdateDynamicBuffer2( const DynBufUpdateParams& params )
  {
    const void* srcBytes = params.mUpdateMemory->GetBytes();
    const int srcByteCount = params.mUpdateMemory->GetByteCount();

    const int iBuf = params.mHandle.GetHandleIndex();
    DX12DynBuf& dynBuf = mDynBufs[ iBuf ];
    void* dstBytes = ( char* )dynBuf.mMappedCPUAddr + params.mByteOffset;

    MemCpy( dstBytes, srcBytes, srcByteCount );
  }

  void DX12Backend::SetRenderObjectName( const SetRenderObjectNameParams& params )
  {
    const int i = params.mHandle.GetHandleIndex();
    const HandleType type = params.mHandle.GetHandleType();
    switch( type )
    {
    case HandleType::kDynamicBuffer: mDynBufs[ i ].SetName( params.mName ); break;
    case HandleType::kContext: mContexts[ i ].SetName( params.mName ); break;
    }
  }
  */
} // namespace Tac::Render