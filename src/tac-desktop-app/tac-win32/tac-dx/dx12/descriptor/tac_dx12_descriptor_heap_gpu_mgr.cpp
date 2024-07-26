#include "tac_dx12_descriptor_heap_gpu_mgr.h" // self-inc
#include "tac-std-lib/os/tac_os.h"

#define TAC_GPU_REGION_DEBUG() 0
#if TAC_GPU_REGION_DEBUG()
//#define TAC_GPU_REGION_DEBUG_TYPE D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
#define TAC_GPU_REGION_DEBUG_TYPE D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
#endif

namespace Tac::Render
{

  StringView DX12DescriptorRegionManager::RegionDesc::StateToString( State state )
  {
    switch( state )
    {
    case State::kUnknown:     return "unknown";
    case State::kAllocated:   return "allocated";
    case State::kFree:        return "free";
    case State::kPendingFree: return "pending free";
    default: TAC_ASSERT_INVALID_CASE( state ); return "?";
    }
  }

  // -----------------------------------------------------------------------------------------------

  DX12DescriptorRegion::DX12DescriptorRegion( DX12Descriptor desc,
                                              DX12DescriptorRegionManager* mgr,
                                              DX12DescriptorRegionManager::RegionIndex regionIndex )
  {
    ( DX12Descriptor& )( *this ) = desc;
    mRegionManager = mgr;
    mRegionIndex = regionIndex;
  }


  void DX12DescriptorRegion::operator = ( DX12DescriptorRegion&& other )
  {
      SwapWith( move( other ) );
  }

    DX12DescriptorRegion::DX12DescriptorRegion( DX12DescriptorRegion&& other )
    {
      SwapWith( move( other ) );
    }

    void DX12DescriptorRegion::SwapWith( DX12DescriptorRegion&& other )
    {
      Swap( ( DX12Descriptor& )( *this ), ( DX12Descriptor& )other );
      Swap( mRegionIndex, other.mRegionIndex );
      Swap( mRegionManager, other.mRegionManager );
    }

    DX12DescriptorRegion::~DX12DescriptorRegion()
    {
      if( !mRegionManager )
        return;

      DX12DescriptorRegionManager::RegionDesc * regionDesc{
        mRegionManager->GetRegionAtIndex( mRegionIndex ) };

      if( !regionDesc )
        return;

      if( regionDesc->mState == DX12DescriptorRegionManager::RegionDesc::kAllocated )
      {
        mRegionManager->Free( regionDesc );
      }
    }

    DX12DescriptorRegionManager::RegionIndex DX12DescriptorRegion::GetRegionIndex() const
    {
      return mRegionIndex;
    }

    void DX12DescriptorRegion::SetFence( FenceSignal fenceSignal )
    {

      DX12DescriptorRegionManager::RegionDesc* regionDesc{
        mRegionManager->GetRegionAtIndex( mRegionIndex ) };

      TAC_ASSERT( regionDesc->mState == DX12DescriptorRegionManager::RegionDesc::kAllocated );
      regionDesc->mState = DX12DescriptorRegionManager::RegionDesc::kPendingFree;
      regionDesc->mFence = fenceSignal;

      //------------------
      if( true )
      {
        auto iToAdd{ mRegionManager->GetIndex( regionDesc ) };
        for( auto iExisting : mRegionManager->mPendingFreeNodes )
        {
          TAC_ASSERT( iToAdd != iExisting );
        }
      }
      //------------------

      mRegionManager->mPendingFreeNodes.push_back( mRegionManager->GetIndex( regionDesc ) );
      mRegionManager = nullptr;
      mRegionIndex = DX12DescriptorRegionManager::RegionIndex::kNull;
    }

  // -----------------------------------------------------------------------------------------------

  void DX12DescriptorRegionManager::RegionDesc::PosInsertAfter( RegionDesc* left,
                                                                DX12DescriptorRegionManager* mgr )
  {
    const RegionIndex iMiddle{ mgr->GetIndex( this ) };
    const RegionIndex iRight{ left->mRightIndex };
    const RegionIndex iLeft{ mgr->GetIndex( left ) };

    left->mRightIndex = iMiddle;
    RegionDesc * right{ mgr->GetRegionAtIndex( iRight ) };
    if( right )
      right->mLeftIndex = iMiddle;

    mLeftIndex = iLeft;
    mRightIndex = iRight;
  }

  void DX12DescriptorRegionManager::RegionDesc::PosRemove(  DX12DescriptorRegionManager* mgr )
  {
    if( RegionDesc * left{ mgr->GetRegionAtIndex( mLeftIndex ) } )
    {
      left->mRightIndex = mRightIndex;
    }

    if( RegionDesc * right{ mgr->GetRegionAtIndex( mRightIndex ) } )
    {
      right->mLeftIndex = mLeftIndex;
    }

    mLeftIndex = DX12DescriptorRegionManager::RegionIndex::kNull;
    mRightIndex = DX12DescriptorRegionManager::RegionIndex::kNull;
  }


  // -----------------------------------------------------------------------------------------------


  void DX12DescriptorRegionManager::Init( Params params )
  {
    mOwner = params.mDescriptorHeap; 
    mCommandQueue = params.mCommandQueue ;
    TAC_ASSERT( mOwner );
    TAC_ASSERT( mCommandQueue );

    const int descriptorCount{ ( int )mOwner->GetDescriptorCount() };
    TAC_ASSERT( descriptorCount );

    mRegions.resize( 1 );
    mRegions[ 0 ] = RegionDesc
    {
      .mLeftIndex       { RegionIndex::kNull },
      .mRightIndex      { RegionIndex::kNull },
      .mDescriptorIndex {},
      .mDescriptorCount { descriptorCount },
      .mState           { RegionDesc::kFree },
      .mFence           {}
    };

    mFreeNodes.push_back( ( RegionIndex )0 );
  }

  DX12DescriptorRegionManager::RegionIndex DX12DescriptorRegionManager::GetIndex( RegionDesc* region ) const
  {
    return RegionIndex( region - mRegions.data() );
  }

  DX12DescriptorRegion DX12DescriptorRegionManager::Alloc( const int descriptorCount )
  {
    if( ++mPump % 8 == 0 ) { PumpFreeQueue(); }

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      OS::OSDebugPrintLine( "alloc "
                            + ToString( descriptorCount )
                            + ( descriptorCount == 1 ? " byte" : " bytes") );
#endif

    RegionIndex iAlloc{ -1 };
    RegionDesc* allocRegion{};

    const int nFree{ mFreeNodes.size() };
    for( int iiFree{}; iiFree < nFree; ++iiFree )
    {
      RegionIndex iFree{ mFreeNodes[ iiFree ] };
      RegionDesc* curr{ GetRegionAtIndex( iFree ) };

      if( curr->mDescriptorCount >= descriptorCount )
      {
        mFreeNodes[ iiFree ] = mFreeNodes[ nFree - 1 ];
        mFreeNodes.pop_back();
        allocRegion = curr;
        iAlloc = iFree;
        break;
      }
    }

    if( !allocRegion )
    {
      if constexpr( IsDebugMode )
      {
        OS::OSDebugBreak();

        DebugPrint();
        ++asdf;
      }

      return {};
    }

    if( allocRegion->mDescriptorCount > descriptorCount )
    {
      RegionIndex iExtra{ RegionIndex::kNull };
      if( mUnusedNodes.empty() )
      {
        iExtra = ( RegionIndex )mRegions.size();
        mRegions.resize( ( int )iExtra + 1 );

        // resize may invalidate pointers (bleh!)
        allocRegion = GetRegionAtIndex( iAlloc );
      }
      else
      {
        iExtra = mUnusedNodes.back();
        mUnusedNodes.pop_back();
      }

      RegionDesc* extraRegion{ GetRegionAtIndex( iExtra ) };
      *extraRegion = RegionDesc
      {
        .mDescriptorIndex { allocRegion->mDescriptorIndex + descriptorCount },
        .mDescriptorCount { allocRegion->mDescriptorCount - descriptorCount },
        .mState           { RegionDesc::State::kFree },
      };

      allocRegion->mDescriptorCount = descriptorCount;

      extraRegion->PosInsertAfter( allocRegion, this );
      mFreeNodes.push_back( iExtra );
    }

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugPrint();
#endif

    allocRegion->mState = RegionDesc::kAllocated;

    const DX12Descriptor descriptor
    {
      .mOwner { mOwner },
      .mIndex { allocRegion->mDescriptorIndex },
      .mCount { allocRegion->mDescriptorCount },
    };

    return DX12DescriptorRegion( descriptor, this, iAlloc );
  }

  String DX12DescriptorRegionManager::DebugPendingFreeListString()
  {
    const FenceSignal lastCompleted{ mCommandQueue->GetLastCompletedFenceValue() };

    String str;
    str += "Completed Fence: " + ToString( lastCompleted.GetValue() ) + "\n";
    str += ToString( mPendingFreeNodes.size() ) + " nodes pending free\n";
    str += "Pending free list: \n";

    int loopCount{};
    for( RegionIndex i : mPendingFreeNodes )
    {
      ++loopCount;
      TAC_ASSERT( loopCount <= ( int )mOwner->GetDescriptorCount() );
      TAC_ASSERT( loopCount <= mPendingFreeNodes.size() );
      str += "[";
      str += "idx: " + ToString( ( int )i ) + ", ";
      str += "fence: " + ToString( GetRegionAtIndex( i )->mFence.GetValue() );
      str += "]\n";
    }
    return str;
  }

  String DX12DescriptorRegionManager::DebugFreeListString()
  {
    String str;
    str += "( free list: ";

    String sep{ "" };
    int loopCount{};
    for( RegionIndex i : mFreeNodes )
    {
      ++loopCount;
      TAC_ASSERT( loopCount <= ( int )mOwner->GetDescriptorCount() );
      str += sep;
      str += ToString( ( int )i );
      sep = "->";
    }
    str += " ) ";
    return str;
  }

  void DX12DescriptorRegionManager::DebugPrint()
  {

    String str;
    str += "------------------------------------------\n";
    str += mOwner->GetName() + ": ";
    OS::OSDebugPrint( str );
    OS::OSDebugPrintLine( DebugFreeListString() );
    OS::OSDebugPrintLine( DebugPendingFreeListString() );

    str.clear();

    int loopCount {};
    for( RegionDesc* desc{ &mRegions[ 0 ] }; desc; desc = GetRegionAtIndex( desc->mRightIndex ) )
    {
      ++loopCount;
      TAC_ASSERT( loopCount <= ( int )mOwner->GetDescriptorCount() );
      TAC_ASSERT( desc->mState != RegionDesc::kUnknown );

      struct
      {
        void AddProperty( StringView name, StringView desc )
        {
          mString += mSep;
          mString += name;
          mString += ": ";
          mString += desc;
          mSep = ", ";
        }

        String mSep{ "" };
        String mString;
      } properties;

      properties.AddProperty( "region", Tac::ToString( ( int )GetIndex( desc ) ) );
      properties.AddProperty( "descriptor index", Tac::ToString( desc->mDescriptorIndex ) );
      properties.AddProperty( "descriptor count", Tac::ToString( desc->mDescriptorCount ) );
      properties.AddProperty( "state", RegionDesc::StateToString( desc->mState ) );
      if( desc->mState == RegionDesc::kPendingFree )
        properties.AddProperty( "fence", Tac::ToString( desc->mFence.GetValue() ) );

      str += "[" + properties.mString + "]";

      OS::OSDebugPrintLine( str );
      str.clear();
    }

    OS::OSDebugPrintLine("");
  }

  void DX12DescriptorRegionManager::PumpFreeQueue()
  {
    int iiRegion{};
    int n{ mPendingFreeNodes.size() };
    while( iiRegion < n )
    {
      RegionIndex iRegion{ mPendingFreeNodes[ iiRegion ] };
      RegionDesc* desc{ GetRegionAtIndex( iRegion ) };
      if( mCommandQueue->IsFenceComplete( desc->mFence ) )
      {
        mPendingFreeNodes[ iiRegion ] = mPendingFreeNodes[ --n ];
        Free( desc );
      }
      else
      {
        ++iiRegion;
      }
    }
    mPendingFreeNodes.resize( n );
  }

  DX12DescriptorRegionManager::RegionDesc* DX12DescriptorRegionManager::GetRegionAtIndex( RegionIndex index )
  {
    return index == RegionIndex::kNull ? nullptr : &mRegions[ ( int )index ];
  }

  void DX12DescriptorRegionManager::Free( RegionDesc* region )
  {

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
    {
      RegionIndex index{ GetIndex( region ) };
      OS::OSDebugPrintLine( "free idx " + ToString( ( int )index ) );
      ++asdf;
    }
#endif

    // was considered used till now (fence signalled)
    TAC_ASSERT( region->mDescriptorCount > 0 );

    bool regionInFreeList{};

    // Merge ourself into the left region
    RegionDesc* left{ GetRegionAtIndex( region->mLeftIndex ) };
    if( left && left->mState == RegionDesc::kFree )
    {
      left->mDescriptorCount += region->mDescriptorCount;
      mUnusedNodes.push_back( GetIndex( region ) );
      region->PosRemove( this );
      *region = {};

      region = left;

      // Since we merged into left, which was in the free list, we are now in the free list
      regionInFreeList = true;
    }

    // Merge the right region into ourself
    RegionDesc* right{ GetRegionAtIndex( region->mRightIndex ) };
    if( right && right->mState == RegionDesc::kFree )
    {
      region->mDescriptorCount += right->mDescriptorCount;
      mUnusedNodes.push_back( region->mRightIndex );
      right->PosRemove( this );
      *right = {};

      RemoveFromFreeList( right );
    }

    if( region->mState != RegionDesc::kFree )
      region->mState = RegionDesc::kFree;

    if( !regionInFreeList )
      mFreeNodes.push_back( GetIndex( region ) );

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugPrint();

    for( RegionDesc* desc{ &mRegions[ 0 ] }; desc; desc = GetRegionAtIndex( desc->mRightIndex ) )
    {
      TAC_ASSERT( desc->mState != RegionDesc::kUnknown );
    }
#endif

  }

  void DX12DescriptorRegionManager::RemoveFromFreeList( RegionDesc* toRemove )
  {
    const RegionIndex iToRemove{ GetIndex( toRemove ) };
    const int n { mFreeNodes.size() };
    for( int i{}; i < n; ++i )
    {
      if( mFreeNodes[ i ] == iToRemove )
      {
        mFreeNodes[i] = mFreeNodes.back();
        mFreeNodes.pop_back();
        return;
      }
    }

    TAC_ASSERT_INVALID_CODE_PATH;
  }

} // namespace Tac::Render

