#pragma once

#include <d3d12.h> // ID3D12...

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"
#include "tac-dx/dx12/resource/tac_dx12_resource.h"


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

#if 0
    PCom< ID3D12Resource >    mBuffer        {};
    D3D12_RESOURCE_STATES     mResourceState {};
#else
    DX12Resource              mResource      {};
#endif
    D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr       {};
    void*                     mCPUAddr       {};
    int                       mByteCount     {};
  };

  struct DX12UploadPageMgr
  {
    auto RequestPage( int byteCount, Errors& ) -> DX12UploadPage;
    void RetirePage( DX12UploadPage, FenceSignal );

  private:
    auto AllocateNewPage( int byteCount, Errors& ) -> DX12UploadPage;
    void UnretirePages();
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
#if 0
      ID3D12Resource*           mResource          {};
      D3D12_RESOURCE_STATES*    mResourceState     {};
      u64                       mResourceOffest    {};
      D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr           {}; // already offset
      void*                     mCPUAddr           {}; // already offset
      void*                     mUnoffsetCPUAddr   {};
      int                       mByteCount         {}; // Bytes for just this allocation
      int                       mResourceByteCount {}; // Bytes for this whole resource
#else
      DX12UploadAllocator*      mParent            {};
      int                       mActivePageIndex   { -1 };
      int                       mLargePageIndex    { -1 };
      int                       mByteCount         {}; // Bytes for just this allocation
      u64                       mResourceOffest    {};
      D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr           {}; // already offset
      void*                     mCPUAddr           {}; // already offset


      auto GetUploadPage() const -> DX12UploadPage*
      {
        if( mLargePageIndex != -1 )
          return &mParent->mLargePages[ mLargePageIndex ];
        if( mActivePageIndex != -1 )
          return &mParent->mActivePages[ mActivePageIndex ];
        TAC_ASSERT_INVALID_CODE_PATH;
        return nullptr;
      }

      //ID3D12Resource* GetResourcePtr() const
      //{
      //  DX12UploadPage* page{ GetUploadPage() };
      //  return page->mResource.Get();
      //}

      auto GetResource() const -> DX12Resource&
      {
        DX12UploadPage* page{ GetUploadPage() };
        return page->mResource;
      }

      //D3D12_RESOURCE_STATES GetResourceStates() const
      //{
      //  DX12UploadPage* page{ GetUploadPage() };
      //  return page->mResource.GetState();
      //}

#endif

    };

    DX12UploadAllocator() = default;
    DX12UploadAllocator( const DX12UploadAllocator& ) = delete;

    void Init( DX12UploadPageMgr* );
    auto Allocate( int, Errors& ) -> DynAlloc;
    void FreeAll( FenceSignal );

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
