#pragma once

#include <d3d12.h> // ID3D12...

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"

#include "tac_dx12_tutorial_command_queue.h"

namespace Tac::Render
{

  struct DX12ExampleGPUUploadPage
  {
    PCom< ID3D12Resource >    mBuffer;

    D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr {};
    void*                     mCPUAddr { nullptr };
    int                       mByteCount {};


    static const int          kDefaultByteCount { 2 * 1024 * 1024 };
  };

  struct DX12ExampleGPUUploadPageManager
  {
    void Init( ID3D12Device* device, DX12ExampleCommandQueue* commandQueue )
    {
      m_device = device;
      mCommandQueue = commandQueue;
    };

    DX12ExampleGPUUploadPage RequestPage( int byteCount, Errors& );
    void          UnretirePages();
    void          RetirePage( DX12ExampleGPUUploadPage, FenceSignal );

  private:
    DX12ExampleGPUUploadPage AllocateNewPage( int byteCount, Errors& );

    struct RetiredPage
    {
      DX12ExampleGPUUploadPage mPage  {};
      FenceSignal              mFence {};
    };

    Vector< RetiredPage >              mRetiredPages   {};
    Vector< DX12ExampleGPUUploadPage > mAvailablePages {};

    // singletons
    PCom< ID3D12Device >               m_device        {};
    DX12ExampleCommandQueue*           mCommandQueue   {};
  };

  struct DX12ExampleGPUUploadAllocator
  {

    void Init( DX12ExampleGPUUploadPageManager* pageManager )
    {
      mPageManager = pageManager;
    }

    struct DynAlloc
    {
      ID3D12Resource*           mResource       {};
      u64                       mResourceOffest {};
      D3D12_GPU_VIRTUAL_ADDRESS mGPUAddr        {}; // already offset
      void*                     mCPUAddr        {}; // already offset
      int                       mByteCount      {};
    };


    DynAlloc Allocate( int const byteCount, Errors& );
    void     FreeAll( FenceSignal FenceID );

  private:

    int            mCurPageUsedByteCount {};

    // Currently in use by command queues the current frame, memory cannot be freed.
    // The last page is the current page
    Vector< DX12ExampleGPUUploadPage > mActivePages;
    Vector< DX12ExampleGPUUploadPage > mLargePages;

    // singletons
    DX12ExampleGPUUploadPageManager* mPageManager {};
  };
}//namespace Tac::Render
