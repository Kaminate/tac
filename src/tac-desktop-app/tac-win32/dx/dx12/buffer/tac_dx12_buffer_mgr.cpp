#include "tac_dx12_buffer_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"

namespace Tac::Render
{
  void DX12BufferMgr::Init( ID3D12Device* device )
  {
    mDevice = device;
  }

    void DX12BufferMgr::CreateDynBuf( DynBufHandle h, int byteCount, StackFrame sf, Errors& errors)
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
        .Width = ( UINT64 )byteCount,
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

      ID3D12Device* device = mDevice;

      PCom< ID3D12Resource > buffer;
      TAC_DX12_CALL( device->CreateCommittedResource(
        &HeapProps,
        D3D12_HEAP_FLAG_NONE,
        &ResourceDesc,
        DefaultUsage,
        nullptr,
        buffer.iid(),
        buffer.ppv() ) );

      DX12SetName( buffer, sf );

      void* cpuAddr;

      TAC_DX12_CALL( buffer->Map(
        0, // subrsc idx
        nullptr, // nullptr indicates the whole subrsc may be read by cpu
        &cpuAddr ) );

      const int i = h.GetIndex();

      mDynBufs[ i ] = DX12DynBuf
      {
        .mResource = buffer,
        .mMappedCPUAddr = cpuAddr,
      };
    }

    void DX12BufferMgr::UpdateDynBuf( RenderApi::UpdateDynBufParams params )
    {
      DynBufHandle h = params.mHandle;
      DX12DynBuf& dynBuf = mDynBufs[ h.GetIndex() ];
      char* dstBytes = ( char* )dynBuf.mMappedCPUAddr + params.mDstByteOffset;
      MemCpy( dstBytes, params.mSrcBytes, params.mSrcByteCount );
    }

    void DX12BufferMgr::DestroyDynBuf( DynBufHandle h )
    {
    if( h.IsValid() )
      mDynBufs[ h.GetIndex() ] = {};
    }
} // namespace Tac::Render