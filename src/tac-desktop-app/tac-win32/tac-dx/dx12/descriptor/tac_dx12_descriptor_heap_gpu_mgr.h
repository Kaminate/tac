#pragma once

namespace Tac { struct Errors; }

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"
#include "tac-std-lib/containers/tac_inlist.h"

namespace Tac::Render
{
  struct DX12DescriptorRegion;
  struct DX12DescriptorRegionManager
  {
    struct Params
    {
      DX12DescriptorHeap* mDescriptorHeap {};
      DX12CommandQueue*   mCommandQueue   {};
    };

    void                 Init( Params );
    DX12DescriptorRegion Alloc( int );
    void                 PumpFreeQueue();
    //void           Free( DX12Descriptor, FenceSignal );
    //void           FreeNoSignal( DX12Descriptor );

  private:

    friend struct DX12DescriptorRegion;
    enum class RegionIndex : int { kNull = -1 };

    struct RegionDesc
    {
      enum State
      {
        kUnknown = 0, kAllocated, kFree, kPendingFree
      };

      // Either the RegionDesc keeps left/right offsets, or the list must be sorted
      RegionIndex mLeftIndex        { -1 };
      RegionIndex mRightIndex       { -1 };

      int         mDescriptorIndex  {};
      int         mDescriptorCount  {};

      State       mState            {};
      FenceSignal mFence            {};

      void PosInsertAfter( RegionDesc*, DX12DescriptorRegionManager* );
      void PosRemove( DX12DescriptorRegionManager* );
      //RegionDesc* GetLeft( DX12DescriptorRegionManager* );
      //RegionDesc* GetRight( DX12DescriptorRegionManager* );

      static StringView StateToString( State );
    };

    void           Free( RegionDesc* );
    RegionIndex            GetIndex( RegionDesc* ) const;
    //RegionDesc*    GetRegion( DX12Descriptor );
    RegionDesc*    GetRegionAtIndex( RegionIndex );
    void           DebugPrint();
    String         DebugFreeListString();
    void RemoveFromFreeList( RegionDesc* );


    Vector< RegionDesc >   mRegions          {};

    Vector< RegionIndex >          mFreeNodes        {};
    Vector< RegionIndex >          mPendingFreeNodes {};
    Vector< RegionIndex >          mUnusedNodes      {};

    DX12CommandQueue*      mCommandQueue     {};
    DX12DescriptorHeap*    mOwner            {};
    int                    mPump             {};
  };

  struct DX12DescriptorRegion : public DX12Descriptor
  {
    DX12DescriptorRegion() = default;
    DX12DescriptorRegion( DX12Descriptor,
                          DX12DescriptorRegionManager*,
                          DX12DescriptorRegionManager::RegionIndex mRegionIndex );
    DX12DescriptorRegion( DX12DescriptorRegion&& );
    ~DX12DescriptorRegion();

    void operator = ( DX12DescriptorRegion&& );
    void SetFence( FenceSignal );

  private:
    void SwapWith( DX12DescriptorRegion&& );
    DX12DescriptorRegionManager*             mRegionManager {};
    DX12DescriptorRegionManager::RegionIndex mRegionIndex{
      DX12DescriptorRegionManager::RegionIndex::kNull };
  };

} // namespace Tac::Render

