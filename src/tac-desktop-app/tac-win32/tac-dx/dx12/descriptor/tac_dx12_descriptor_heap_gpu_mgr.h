#pragma once

namespace Tac { struct Errors; }

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"


namespace Tac::Render
{
  struct DX12DescriptorRegionManager
  {
    struct Params
    {
      DX12DescriptorHeap* mDescriptorHeap {};
      DX12CommandQueue*   mCommandQueue   {};
    };
    void           Init( Params );
    DX12Descriptor Alloc( int );
    void           PumpFreeQueue();
    void           Free( DX12Descriptor, FenceSignal );
    void           FreeNoSignal( DX12Descriptor );

  private:
    
    struct RegionDesc
    {
      // In-use region descs point to dummy instead of nullptr
      RegionDesc* mLeft     {};
      RegionDesc* mRight    {};
      RegionDesc* mNextFree {}; 
      RegionDesc* mPrevFree {}; 
      int         mSize     {};
    };

    void           Free( RegionDesc* );
    int            GetIndex( RegionDesc* ) const;
    RegionDesc*    GetRegion( DX12Descriptor );
    void           DebugPrint();
    void           FreeListAdd(RegionDesc*);
    void           FreeListRemove(RegionDesc*);

    struct RegionToFree
    {
      RegionDesc* mRegion {};
      FenceSignal mFence  {};
    };

    Vector< RegionDesc >   mAllRegions    {};
    Vector< RegionToFree > mRegionsToFree {};
    RegionDesc             mDummy         {};
    DX12CommandQueue*      mCommandQueue  {};
    DX12DescriptorHeap*    mOwner         {};
    int                    mPump          {};
  };
} // namespace Tac::Render

