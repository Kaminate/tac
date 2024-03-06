#include "tac_dx12_gpu_upload_allocator.h"

#include "src/common/error/tac_error_handling.h"
#include "src/common/algorithm/tac_algorithm.h"
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"


namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  // GPUUploadAllocator

  GPUUploadAllocator::DynAlloc GPUUploadAllocator::Allocate( int const byteCount, Errors& errors )
  {
    // so the deal with large pages, is that they can't be reused as default pages.
    // so normally, when allocating a page, you first check if a retired page can be reused,
    // but large pages are just deleted when they are no longer used.

    GPUUploadPage* pageToAllocateFrom = nullptr;
    if( byteCount > GPUUploadPage::kDefaultByteCount )
    {
      GPUUploadPage requested =
        TAC_CALL_RET( {}, mPageManager->RequestPage( byteCount, errors ) );
      mLargePages.push_back( requested );
      pageToAllocateFrom = &mLargePages.back();
    }

    // see if the allocation will fit in the current page
    if( !pageToAllocateFrom && !mActivePages.empty() )
    {
      GPUUploadPage* curPage = &mActivePages.back();

      // if the allocation doesn't fit this page will be retired
      mCurPageUsedByteCount = RoundUpToNearestMultiple( mCurPageUsedByteCount, byteCount );

      if( byteCount <= curPage->mByteCount - mCurPageUsedByteCount )
      {
        pageToAllocateFrom = curPage;
      }
    }

    if( !pageToAllocateFrom )
    {
        GPUUploadPage requested =
          TAC_CALL_RET( {}, mPageManager->RequestPage( GPUUploadPage::kDefaultByteCount, errors ) );
        mActivePages.push_back( requested );

        TAC_ASSERT_MSG( mActivePages.size() < 100, "why do you have so many pages bro" );

        mCurPageUsedByteCount = 0;
        pageToAllocateFrom = &mActivePages.back();
    }

    TAC_ASSERT( pageToAllocateFrom );

    const u64 offset = mCurPageUsedByteCount;
    mCurPageUsedByteCount += byteCount;

    D3D12_GPU_VIRTUAL_ADDRESS const gpuAddr = pageToAllocateFrom->mGPUAddr + offset;
    void* const cpuAddr = ( u8* )pageToAllocateFrom->mCPUAddr + offset;


    return DynAlloc
    {
      .mResource = pageToAllocateFrom->mBuffer.Get(),
      .mResourceOffest = offset,
      .mGPUAddr = gpuAddr,
      .mCPUAddr = cpuAddr,
      .mByteCount = byteCount,
    };
  }

  // call at end of each context
  void GPUUploadAllocator::FreeAll( FenceSignal FenceID )
  {
    for( const GPUUploadPage& page : mActivePages )
    {
      mPageManager->RetirePage( page, FenceID );
    }

    for( const GPUUploadPage& page : mLargePages )
    {
      mPageManager->RetirePage( page, FenceID );
    }

    mActivePages.clear();
    mCurPageUsedByteCount = 0;
    // ...
  }

  // -----------------------------------------------------------------------------------------------

  // GPUUploadPageManager

  void GPUUploadPageManager::UnretirePages()
  {
    int n = mRetiredPages.size();
    if( !n )
      return;

    int i = 0;
    while( i < n )
    {
      RetiredPage& currPage = mRetiredPages[ i ];
      RetiredPage& backPage = mRetiredPages[ n - 1 ];

      const FenceSignal fenceValue = currPage.mFence;
      if( mCommandQueue->IsFenceComplete( fenceValue ) )
      {
        if( currPage.mPage.mByteCount <= GPUUploadPage::kDefaultByteCount )
        {
          mAvailablePages.push_back( currPage.mPage );
        }

        Swap( currPage, backPage );
        backPage = {};
        --n;
      }
      else
      {
        // Break instead of ++i.
        // If earlier pages fences arent complete, later pages wont be either.
        break;
      }
    }

    mRetiredPages.resize( n );
  }

  GPUUploadPage GPUUploadPageManager::RequestPage( int byteCount, Errors& errors )
  {
    UnretirePages();

    GPUUploadPage page{};
    if( mAvailablePages.empty() )
    {
      page = AllocateNewPage( byteCount, errors );
    }
    else
    {
      page = mAvailablePages.back();
      mAvailablePages.pop_back();
    }

    return page;

  }

  GPUUploadPage GPUUploadPageManager::AllocateNewPage( int byteCount, Errors& errors )
  {
    //TAC_ASSERT( !mCurPage.IsValid() );
    TAC_ASSERT( byteCount == GPUUploadPage::kDefaultByteCount );


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

    return GPUUploadPage
    {
      .mBuffer = buffer,
      .mGPUAddr = buffer->GetGPUVirtualAddress(),
      .mCPUAddr = cpuAddr,
      .mByteCount = byteCount,
    };

  }

  void          GPUUploadPageManager::RetirePage( GPUUploadPage page, FenceSignal signal )
  {
    RetiredPage retired{ .mPage = page, .mFence = signal };
    mRetiredPages.push_back( retired );
  }


  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Render
