// The DX12DescriptorRegionManager manages the descriptor heaps of a DX12DescriptorHeap by
// implementing a memory manangement system of a heap.
//
// Memory is allocated in the form of DX12DescriptorRegion, which is a DX12DescriptorRegion that
// works with the DX12DescriptorRegionManager.
// 
// When a DX12DescriptorRegion SetFence() is called, the memory region is marked as pending free,
// and only becomes truly free when the command queue signals the fence.
//
// Regions of contiguous descriptors are referred to as DX12DescriptorRegionManager::RegionDesc,
// and when a region is freed, it merges with adjacent free regions
//
// tl;dr: This file implements a heap for gpu-visible descriptor indexes

#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"

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
      RegionIndex mLeftIndex        { RegionIndex::kNull };
      RegionIndex mRightIndex       { RegionIndex::kNull };

      int         mDescriptorIndex  {};
      int         mDescriptorCount  {};

      State       mState            { kUnknown };
      FenceSignal mFence            {};

      void PosInsertAfter( RegionDesc*, DX12DescriptorRegionManager* );
      void PosRemove( DX12DescriptorRegionManager* );

      static StringView StateToString( State );
    };

    void           Free( RegionDesc* );
    RegionIndex    GetIndex( RegionDesc* ) const;
    RegionDesc*    GetRegionAtIndex( RegionIndex );
    void           DebugPrint();
    String         DebugFreeListString();
    String         DebugPendingFreeListString();
    void           RemoveFromFreeList( RegionDesc* );


    Vector< RegionDesc >   mRegions          {};

    Vector< RegionIndex >  mFreeNodes        {};
    Vector< RegionIndex >  mPendingFreeNodes {};
    Vector< RegionIndex >  mUnusedNodes      {};

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
    void operator = ( const DX12DescriptorRegion& ) = delete;
    void SetFence( FenceSignal );

    DX12DescriptorRegionManager::RegionIndex GetRegionIndex() const { return mRegionIndex; }

  private:
    void SwapWith( DX12DescriptorRegion&& );
    DX12DescriptorRegionManager*             mRegionManager {};
    DX12DescriptorRegionManager::RegionIndex mRegionIndex{
      DX12DescriptorRegionManager::RegionIndex::kNull };
  };

} // namespace Tac::Render

