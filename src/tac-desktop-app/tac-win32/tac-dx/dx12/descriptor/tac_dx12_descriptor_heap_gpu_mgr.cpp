#include "tac_dx12_descriptor_heap_gpu_mgr.h" // self-inc

#define TAC_GPU_REGION_DEBUG() 0
#if TAC_GPU_REGION_DEBUG()
#include "tac-std-lib/os/tac_os.h"
#define TAC_GPU_REGION_DEBUG_TYPE D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
#endif

namespace Tac::Render
{
  // prepends to front of list
  void           DX12DescriptorRegionManager::FreeListAdd( RegionDesc* region )
  {
    TAC_ASSERT( !region->mNextFree );
    TAC_ASSERT( !region->mPrevFree );

    RegionDesc* prev{ &mDummy };
    RegionDesc* next{ mDummy.mNextFree };

    prev->mNextFree = region;
    next->mPrevFree = region;
    region->mPrevFree = prev;
    region->mNextFree = next;
  }

  void           DX12DescriptorRegionManager::FreeListRemove( RegionDesc* region )
  {
    TAC_ASSERT( region->mNextFree );
    TAC_ASSERT( region->mPrevFree );

    RegionDesc* prev{ region->mPrevFree };
    RegionDesc* next{ region->mNextFree };

    prev->mNextFree = next;
    next->mPrevFree = prev;
    region->mNextFree = nullptr;
    region->mPrevFree = nullptr;
  }

  void DX12DescriptorRegionManager::Init( Params params )
  {
    mOwner = params.mDescriptorHeap; 
    mCommandQueue = params.mCommandQueue ;
    TAC_ASSERT( mOwner );
    TAC_ASSERT( mCommandQueue );

    const int size{ ( int )mOwner->GetDescriptorCount() };
    TAC_ASSERT( size );
    mAllRegions.resize( size );
    RegionDesc* region{ &mAllRegions[ 0 ] };
    *region = RegionDesc
    {
      .mLeft     { &mDummy },
      .mRight    { &mDummy },
      .mSize     { size },
    };
    mDummy = RegionDesc
    {
      .mLeft     { region },
      .mRight    { region },
      .mNextFree { &mDummy },
      .mPrevFree { &mDummy },
      .mSize     { 999999 }, // easier to see in debugger
    };

    FreeListAdd( region );

  }

  int DX12DescriptorRegionManager::GetIndex( RegionDesc* region ) const
  {
    return int( region - mAllRegions.data() );
  }

  DX12Descriptor DX12DescriptorRegionManager::Alloc( const int n )
  {
    if( ++mPump % 8 == 0 ) { PumpFreeQueue(); }

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      OS::OSDebugPrintLine( "alloc " + ToString( n ) + ( n == 1 ? " byte" : " bytes") );
#endif

    RegionDesc* currRegion{ mDummy.mNextFree };
    for( ;; )
    {
      if( currRegion == &mDummy || currRegion->mSize >= n )
        break;

      currRegion = currRegion->mRight;
    }

    if( currRegion == &mDummy )
      return DX12Descriptor{};

    const int iCurr{ GetIndex( currRegion ) };
    if( currRegion->mSize > n )
    {
      const int iExtra{ iCurr + n };
      RegionDesc* extraRegion{ &mAllRegions[ iExtra ] };
      *extraRegion = RegionDesc
      {
        .mLeft     { currRegion },
        .mRight    { currRegion->mRight },
        .mSize     { currRegion->mSize - n },
      };

      currRegion->mSize = n;
      currRegion->mRight->mLeft = extraRegion;
      currRegion->mRight = extraRegion;

      FreeListAdd( extraRegion );

    }

    FreeListRemove( currRegion );
    
#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugPrint();
#endif

    return DX12Descriptor
    {
      .mOwner { mOwner },
      .mIndex { iCurr },
      .mCount { n },
    };
  }

  void DX12DescriptorRegionManager::DebugPrint()
  {
#if TAC_GPU_REGION_DEBUG()

    String str;
    str += mOwner->GetName() + ": ";

    auto InFreeList = [& ]( RegionDesc* query )
    {
      for( RegionDesc* desc{ mDummy.mNextFree }; desc != &mDummy; desc = desc->mNextFree )
        if( query == desc )
          return true;
      return false;
    };

    str += "( free list: ";
    String sep{""};
    for( RegionDesc* desc{ mDummy.mNextFree }; desc != &mDummy; desc = desc->mNextFree )
    {
      const int i{ GetIndex( desc ) };
      TAC_ASSERT( str.size() < 1000 );
      str += sep;
      str += ToString( i );
      sep = " --> ";
    }
    str += " ) ";

    RegionDesc* left{ &mDummy };
    for( RegionDesc* desc{ mDummy.mRight }; desc != &mDummy; desc = desc->mRight )
    {
      const int i{ GetIndex( desc ) };
      const bool inFreeList{ InFreeList( desc ) };
      str += "idx " + Tac::ToString( i ) + "[" +
        Tac::ToString( desc->mSize ) + " byte " +
        ( desc->mNextFree ? "free" : "allocated" ) + 
        ( inFreeList ? String(", next free: ") + ToString( GetIndex(desc->mNextFree) ) : String("") ) +
        "] ";

      TAC_ASSERT( (bool)desc->mNextFree == inFreeList );

      TAC_ASSERT( desc->mLeft == left );
      left = desc;
    }

    OS::OSDebugPrintLine(str);
#endif
  }

  void DX12DescriptorRegionManager::PumpFreeQueue()
  {
    int i{};
    int n{ mRegionsToFree.size() };
    while( i < n )
    {
      RegionToFree toFree{ mRegionsToFree[ i ] };
      if( mCommandQueue->IsFenceComplete( toFree.mFence ) )
      {
        Free( toFree.mRegion );

        mRegionsToFree[ i ] = mRegionsToFree[ --n ];
      }
      else
        ++i;
    }
    mRegionsToFree.clear();
  }

  void DX12DescriptorRegionManager::Free( DX12Descriptor descriptor, FenceSignal fenceSignal )
  {
    RegionDesc* region{ GetRegion( descriptor ) };

#if TAC_GPU_REGION_DEBUG()
    for( RegionToFree regionToFree : mRegionsToFree )
    {
      TAC_ASSERT( regionToFree.mRegion != region );
    }
#endif

    const RegionToFree regionToFree
    {
      .mRegion { region },
      .mFence  { fenceSignal },
    };
    mRegionsToFree.push_back( regionToFree );
  }

  DX12DescriptorRegionManager::RegionDesc* DX12DescriptorRegionManager::GetRegion(
    DX12Descriptor descriptor )
  {
    TAC_ASSERT( descriptor.Valid() );
    TAC_ASSERT( descriptor.mOwner == mOwner );
    TAC_ASSERT( descriptor.mCount > 0 );
    TAC_ASSERT_INDEX( descriptor.mIndex, mAllRegions.size() );

    RegionDesc* region{ &mAllRegions[ descriptor.mIndex ] };

    TAC_ASSERT( !region->mNextFree );
    TAC_ASSERT( region->mSize == descriptor.mCount );
    TAC_ASSERT( region->mLeft );
    TAC_ASSERT( region->mRight );

    return region;
  }

  void DX12DescriptorRegionManager::FreeNoSignal( DX12Descriptor descriptor )
  {
    RegionDesc* region{ GetRegion( descriptor ) };
    Free( region );
  }

  void DX12DescriptorRegionManager::Free( RegionDesc* region )
  {

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
    {
      int index{ GetIndex( region ) };
      OS::OSDebugPrintLine( "free idx " + ToString( index ) );
      ++asdf;
      if( index == 0 )
        ++asdf;
    }
#endif

    // was considered used till now (fence signalled)
    TAC_ASSERT( region->mSize > 0 );
    TAC_ASSERT( !region->mNextFree );
    TAC_ASSERT( region->mLeft );
    TAC_ASSERT( region->mRight );
    RegionDesc* left{ region->mLeft };
    RegionDesc* right{ region->mRight };
    const bool mergeLeft{ left != &mDummy && left->mNextFree };
    const bool mergeRight{ right != &mDummy && right->mNextFree };
    bool regionInFreeList{ false };
    if( mergeLeft )
    {
      left->mSize += region->mSize;
      left->mRight = region->mRight;
      left->mRight->mLeft = left;
      *region = { };
      region = left;

      // Since we merged into left, which was in the free list, we are now in the free list
      regionInFreeList = true;
    }

    if( mergeRight )
    {
      FreeListRemove( right );

      region->mSize += right->mSize;
      region->mRight = right->mRight;
      region->mRight->mLeft = region;

      *right = {};
    }

    if( !regionInFreeList )
      FreeListAdd( region );

    TAC_ASSERT( region->mNextFree->mPrevFree == region );
    TAC_ASSERT( region->mPrevFree->mNextFree == region );

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugPrint();
#endif
  }

} // namespace Tac::Render

