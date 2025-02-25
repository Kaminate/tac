#pragma once

#include <d3d12.h> // ID3D12...

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"

#include "tac_dx12_command_queue.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <mutex>
#endif

namespace Tac::Render
{
  struct DX12UploadPage
  {
    static const int          kDefaultByteCount { 2 * 1024 * 1024 };

    PCom< ID3D12Resource >    mBuffer;
    D3D12_RESOURCE_STATES     mResourceState {};
    D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr       {};
    void*                     mCPUAddr       {};
    int                       mByteCount     {};
  };

  struct DX12UploadPageMgr
  {
    DX12UploadPage RequestPage( int byteCount, Errors& );
    void           RetirePage( DX12UploadPage, FenceSignal );

  private:
    DX12UploadPage AllocateNewPage( int byteCount, Errors& );
    void           UnretirePages();
    //void           FreeLargePages();

    struct RetiredPage
    {
      DX12UploadPage mPage;
      FenceSignal    mFence{};
    };

    Vector< RetiredPage >    mRetiredPages; // Retired pages get recycled
    //Vector< RetiredPage >    mRetiredLargePages; // Retired large pages get deleted
    Vector< DX12UploadPage > mAvailablePages;

    std::mutex               mPagesMutex;
  };

  struct DX12UploadAllocator
  {
    struct DynAlloc
    {
      ID3D12Resource*           mResource          {};
      D3D12_RESOURCE_STATES*    mResourceState     {};
      u64                       mResourceOffest    {};
      D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr           {}; // already offset
      void*                     mCPUAddr           {}; // already offset
      void*                     mUnoffsetCPUAddr   {};
      int                       mByteCount         {}; // Bytes for just this allocation
      int                       mResourceByteCount {}; // Bytes for this whole resource
    };

    DX12UploadAllocator() = default;
    DX12UploadAllocator( const DX12UploadAllocator& ) = delete;

    void     Init( DX12UploadPageMgr* );
    DynAlloc Allocate( int, Errors& );
    void     FreeAll( FenceSignal );

    void operator = ( const DX12UploadAllocator& ) = delete;

  private:


    // Currently in use by command queues the current frame, memory cannot be freed.
    // The last page is the current page
    Vector< DX12UploadPage > mActivePages;
    Vector< DX12UploadPage > mLargePages;
    int                      mCurPageUsedByteCount {};

    // singletons
    DX12UploadPageMgr*       mPageManager {};
  };
}//namespace Tac::Render
