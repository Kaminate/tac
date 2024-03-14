// Why this file exists:
//
// In a command list, the commands themselves each have memory.
// After we call ID3D12CommandQueue::ExecuteCommandList() on a command list, we put it's command
// allocator back into the pool. However, we cannot reuse that allocator until the value associated
// with the GPU completion of all commands has been signalled.
//
// tl;dr: DX12ExampleCommandAllocatorPool exists to be able to re-use ID3D12CommandAllocator's

#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_ring_vector.h"
#include  "tac_example_dx12_fence.h"

#include <d3d12.h> // D3D12...

namespace Tac { struct Errors; }
namespace Tac::Render
{
  struct DX12ExampleCommandQueue;

  // a Id3D12CommandAllocator Represents the allocations of storage for graphics processing unit (GPU) commands.



  // wtf is this. this should not be a pool of id3d12graphicscommandlist, it should be a pool of
  // commandlistallocators
  struct DX12ExampleCommandAllocatorPool
  {
    void                           Init( PCom<ID3D12Device >, DX12ExampleCommandQueue* );
    
    void                           Retire( PCom< ID3D12CommandAllocator >, FenceSignal  );
    PCom< ID3D12CommandAllocator > GetAllocator( FenceSignal, Errors&  );
    PCom< ID3D12CommandAllocator > GetAllocator( Errors&  );

  private:
    PCom< ID3D12CommandAllocator > TryReuseAllocator( FenceSignal );
    PCom< ID3D12CommandAllocator > CreateNewCmdAllocator(Errors&);

    struct Element
    {
      PCom< ID3D12CommandAllocator > mCmdAllocator;
      FenceSignal mSignalValue;
    };

    PCom< ID3D12Device5 > m_device; // device4 has createcommandlist1 method
    RingVector< Element > mElements;
    DX12ExampleCommandQueue* mCommandQueue = nullptr;
  };

} // namespace Tac

