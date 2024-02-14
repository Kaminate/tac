#include "tac_example_dx12_gpu_upload_allocator.h"

#include "src/common/error/tac_error_handling.h"
#include "src/common/algorithm/tac_algorithm.h"
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"


namespace Tac::Render
{
  GPUUploadAllocator::DynAlloc GPUUploadAllocator::Allocate( int const byteCount, Errors& errors )
  {
    // so the deal with large pages, is that they can't be reused as default pages.
    // so normally, when allocating a page, you first check if a retired page can be reused,
    // but large pages are just deleted when they are no longer used.
    TAC_ASSERT_MSG( byteCount < Page::kDefaultByteCount, "large pages currently unsupported" );

    TAC_ASSERT_MSG( false, "need to align byteCount and mCurPageUsedByteCount" );

    Page* curPage = mActivePages.empty() ? nullptr : &mActivePages.back();
    if( mActivePages.empty() || byteCount > curPage->mByteCount - mCurPageUsedByteCount )
    {
      TAC_CALL_RET( {}, RequestPage( Page::kDefaultByteCount, errors ) );
    }

    D3D12_GPU_VIRTUAL_ADDRESS const gpuAddr = curPage->mGPUAddr + mCurPageUsedByteCount;
    void* const cpuAddr = ( u8* )curPage->mCPUAddr + mCurPageUsedByteCount;

    mCurPageUsedByteCount += byteCount;

    return DynAlloc
    {
      .mGPUAddr = gpuAddr,
      .mCPUAddr = cpuAddr,
      .mByteCount = byteCount,
    };
  }

  // call at end of each frame
  void GPUUploadAllocator::FreeAll( DX12CommandQueue::Signal FenceID )
  {
    for( const Page& page : mActivePages )
    {
      RetiredPage retiredPage
      {
        .mPage = page,
        .mFence = FenceID,
      };
      mRetiredPages.push_back( retiredPage );
    }
    // ...
  }

  void GPUUploadAllocator::UnretirePages()
  {
    int n = mRetiredPages.size();
    if( !n )
      return;

    int i = 0;
    while( i < n )
    {
      RetiredPage& currPage = mRetiredPages[ i ];
      RetiredPage& backPage = mRetiredPages[ n - 1 ];

      const DX12CommandQueue::Signal fenceValue = currPage.mFence;
      if( mCommandQueue->IsFenceComplete( fenceValue ) )
      {
        mAvailablePages.push_back( currPage.mPage );
        Swap( currPage, backPage );
        --n;
      }
      else
      {
        break;
        //++i;
      }
    }

    mRetiredPages.resize( n );
  }

  void GPUUploadAllocator::RequestPage( int byteCount, Errors& errors )
  {
    UnretirePages();

    Page page{};
    if( mAvailablePages.empty() )
    {
      page = mAvailablePages.back();
      mAvailablePages.pop_back();
    }
    else
    {
      page = AllocateNewPage( byteCount, errors );
    }

    mActivePages.push_back( page );
    mCurPageUsedByteCount = 0;
  }

  GPUUploadAllocator::Page GPUUploadAllocator::AllocateNewPage( int byteCount, Errors& errors )
  {
    //TAC_ASSERT( !mCurPage.IsValid() );
    TAC_ASSERT( byteCount == Page::kDefaultByteCount );


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
      .SampleDesc = DXGI_SAMPLE_DESC{.Count = 1, .Quality = 0 },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    const D3D12_RESOURCE_STATES DefaultUsage{ D3D12_RESOURCE_STATE_GENERIC_READ };

    PCom<ID3D12Resource> buffer;
    TAC_DX12_CALL_RET( {}, m_device->CreateCommittedResource(
      &HeapProps,
      D3D12_HEAP_FLAG_NONE,
      &ResourceDesc,
      DefaultUsage,
      nullptr,
      buffer.iid(),
      buffer.ppv() ) );

    DX12SetName( buffer, "upload page" );

    void* cpuAddr;

    TAC_DX12_CALL_RET( {}, buffer->Map(
      0, // subrsc idx
      nullptr, // nullptr indicates the whole subrsc may be read by cpu
      &cpuAddr ) );

    return Page
    {
      .mBuffer = buffer,
      .mGPUAddr = buffer->GetGPUVirtualAddress(),
      .mCPUAddr = cpuAddr,
      .mByteCount = byteCount,
    };

  }

} // namespace Tac::Render
