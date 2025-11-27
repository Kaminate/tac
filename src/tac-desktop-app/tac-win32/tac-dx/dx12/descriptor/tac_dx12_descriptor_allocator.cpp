#include "tac_dx12_descriptor_allocator.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"

#define TAC_GPU_REGION_DEBUG() TAC_IS_DEBUG_MODE() && 0
#if TAC_GPU_REGION_DEBUG()
//#define TAC_GPU_REGION_DEBUG_TYPE D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
#define TAC_GPU_REGION_DEBUG_TYPE D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
#endif

namespace Tac::Render
{

  StringView DX12DescriptorAllocator::RegionDesc::StateToString( State state )
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
                                              DX12DescriptorAllocator* mgr,
                                              DX12DescriptorAllocator::RegionIndex regionIndex )
  {
    ( DX12Descriptor& )( *this ) = desc;
    mRegionManager = mgr;
    mRegionIndex = regionIndex;
  }


  void DX12DescriptorRegion::operator = ( DX12DescriptorRegion&& other ) noexcept
  {
    SwapWith( move( other ) );
  }

  //void DX12DescriptorRegion::operator = ( const DX12DescriptorRegion& other )
  //{
  //  TAC_ASSERT( !mRegionManager );
  //  TAC_ASSERT( mRegionIndex == DX12DescriptorAllocator::RegionIndex::kNull );
  //  TAC_ASSERT( !other.mRegionManager );
  //  TAC_ASSERT( other.mRegionIndex == DX12DescriptorAllocator::RegionIndex::kNull );
  //}

  DX12DescriptorRegion::DX12DescriptorRegion( DX12DescriptorRegion&& other ) noexcept
  {
    SwapWith( move( other ) );
  }

  DX12DescriptorRegion::~DX12DescriptorRegion()
  {
    if( !mRegionManager )
      return;

    DX12DescriptorAllocator::RegionDesc* regionDesc{
      mRegionManager->GetRegionAtIndex( mRegionIndex ) };

    if( !regionDesc )
      return;

    if( regionDesc->mState == DX12DescriptorAllocator::RegionDesc::kAllocated )
    {
      // What is this code path doing?
      //   Freeing a descriptor region that was not used in a draw call?
      //
      // Should there be a warning?

      mRegionManager->Free( regionDesc );
    }
  }

  void DX12DescriptorRegion::SwapWith( DX12DescriptorRegion&& other )
  {
    TAC_ASSERT( !DX12Descriptor::IsValid() );
    TAC_ASSERT( !mRegionManager );
    TAC_ASSERT( mRegionIndex == DX12DescriptorAllocator::RegionIndex::kNull );
    Swap( ( DX12Descriptor& )( *this ), ( DX12Descriptor& )other );
    Swap( mRegionIndex, other.mRegionIndex );
    Swap( mRegionManager, other.mRegionManager );
  }

  //DX12DescriptorAllocator::RegionIndex DX12DescriptorRegion::GetRegionIndex() const
  //{
  //  return mRegionIndex;
  //}

  void DX12DescriptorRegion::Free( FenceSignal fenceSignal )
  {

#if TAC_GPU_REGION_DEBUG()
    const String dbgTitle{ "SetFence( "
                            "region " + ToString( ( int )mRegionIndex ) + ", "
                            "fence " + ToString( fenceSignal.GetValue() ) + " )" };

    mRegionManager->DebugTitleBegin( dbgTitle );
#endif

    dynmc DX12DescriptorAllocator::RegionDesc* regionDesc{
      mRegionManager->GetRegionAtIndex( mRegionIndex ) };

    TAC_ASSERT( regionDesc->mState == DX12DescriptorAllocator::RegionDesc::kAllocated );
    regionDesc->mState = DX12DescriptorAllocator::RegionDesc::kPendingFree;
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
    mRegionIndex = DX12DescriptorAllocator::RegionIndex::kNull;

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
    {
      mRegionManager->DebugPrint();
    }

    mRegionManager->DebugTitleEnd( dbgTitle );
#endif

    mRegionManager = nullptr;

    mOwner = {};
    mIndex = {};
    mCount = {};
  }

  // -----------------------------------------------------------------------------------------------

  void DX12DescriptorAllocator::RegionDesc::PosInsertAfter( RegionDesc* left,
                                                                DX12DescriptorAllocator* mgr )
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

  void DX12DescriptorAllocator::RegionDesc::PosRemove(  DX12DescriptorAllocator* mgr )
  {
    if( RegionDesc * left{ mgr->GetRegionAtIndex( mLeftIndex ) } )
    {
      left->mRightIndex = mRightIndex;
    }

    if( RegionDesc * right{ mgr->GetRegionAtIndex( mRightIndex ) } )
    {
      right->mLeftIndex = mLeftIndex;
    }

    mLeftIndex = DX12DescriptorAllocator::RegionIndex::kNull;
    mRightIndex = DX12DescriptorAllocator::RegionIndex::kNull;
  }


  // -----------------------------------------------------------------------------------------------


  void DX12DescriptorAllocator::Init( Params params )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    mOwner = params.mDescriptorHeap; 
    mCommandQueue = &renderer.mCommandQueue;
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

  auto DX12DescriptorAllocator::GetIndex( RegionDesc* region ) const -> RegionIndex
  {
    return RegionIndex( region - mRegions.data() );
  }

  void DX12DescriptorAllocator::DebugTitleBegin( StringView title )
  {
#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
    {
      const String str( 100, '-' );
      OS::OSDebugPrintLine( str );
      OS::OSDebugPrint( title );
      OS::OSDebugPrintLine( " begin" );
    }
#else
    TAC_UNUSED_PARAMETER( title );
#endif
  }

  void DX12DescriptorAllocator::DebugTitleEnd( StringView title )
  {
#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
    {
      const String str( 100, '-' );
      OS::OSDebugPrint( title );
      OS::OSDebugPrintLine( " end" );
      OS::OSDebugPrintLine( str );
    }
#else
    TAC_UNUSED_PARAMETER( title );
#endif
  }

  auto DX12DescriptorAllocator::Alloc( const int descriptorCount ) -> DX12DescriptorRegion
  {
#if !TAC_DELETE_ME()

    // -------------------------------------
    // -----------  FIX ME         ---------
    // -------------------------------------

    // There seems to be a memory leak of some sort, where over time, 
    // mRegions and mPendingFreeNodes grows until we run out of descriptors to allocate
    // then the program asserts.

    // -------------------------------------
    // -------------   FIX ME --------------
    // -------------------------------------

#endif
    if( ++mPump % 8 == 0 ) { PumpFreeQueue(); }

#if TAC_GPU_REGION_DEBUG()
    const String dbgTitle{ "alloc "
                     + ToString( descriptorCount )
                     + ( descriptorCount == 1 ? " byte" : " bytes" ) };
    DebugTitleBegin( dbgTitle );
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
      // Make sure that App::Present() and ImGuiPlatformPresent() are not calling
      // the same IDXGISwapChain::Present() twice per render frame.
      if constexpr( kIsDebugMode )
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

    allocRegion->mState = RegionDesc::kAllocated;

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugPrint();

    DebugTitleEnd( dbgTitle + "( region: " + ToString( ( int )iAlloc ) + " )" );
#endif

    const DX12Descriptor descriptor
    {
      .mOwner { mOwner },
      .mIndex { allocRegion->mDescriptorIndex },
      .mCount { allocRegion->mDescriptorCount },
    };

    return DX12DescriptorRegion( descriptor, this, iAlloc );
  }

  auto DX12DescriptorAllocator::DebugPendingFreeListString() -> String
  {
    const FenceSignal lastCompleted{ mCommandQueue->GetLastCompletedFenceValue() };
    const int nPendingFree{ mPendingFreeNodes.size() };

    String str;
    str += "Completed Fence: " + ToString( lastCompleted.GetValue() ) + "\n";
    if( nPendingFree )
    {
      str += "Pending free list begin( " + ToString( nPendingFree ) + " nodes ): \n";
      int loopCount{};
      for( RegionIndex i : mPendingFreeNodes )
      {
        ++loopCount;
        TAC_ASSERT( loopCount <= ( int )mOwner->GetDescriptorCount() );
        TAC_ASSERT( loopCount <= mPendingFreeNodes.size() );
        str += "\t[";
        str += "idx: " + ToString( ( int )i ) + ", ";
        str += "fence: " + ToString( GetRegionAtIndex( i )->mFence.GetValue() );
        str += "]\n";
      }
      str += "Pending free list end\n";
    }
    else
    {
      str += "Pending free list: empty\n";
    }
    return str;
  }

  auto DX12DescriptorAllocator::DebugFreeListString() -> String
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

  void DX12DescriptorAllocator::DebugPrint()
  {
    String str;
    for( int i{}; i < 100; ++i)
      str += "-";
    str += "\n";
    str += mOwner->GetName() + ": ";
    OS::OSDebugPrint( str );
    OS::OSDebugPrintLine( DebugFreeListString() );
    OS::OSDebugPrintLine( DebugPendingFreeListString() );

    str.clear();
    OS::OSDebugPrintLine("Region List Begin");

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

        String mSep;
        String mString;
      } properties;

      properties.AddProperty( "region", Tac::ToString( ( int )GetIndex( desc ) ) );
      properties.AddProperty( "descriptor index", Tac::ToString( desc->mDescriptorIndex ) );
      properties.AddProperty( "descriptor count", Tac::ToString( desc->mDescriptorCount ) );
      properties.AddProperty( "state", RegionDesc::StateToString( desc->mState ) );
      if( desc->mState == RegionDesc::kPendingFree )
        properties.AddProperty( "fence", Tac::ToString( desc->mFence.GetValue() ) );

      str += "\t[" + properties.mString + "]";

      OS::OSDebugPrintLine( str );
      str.clear();
    }

    OS::OSDebugPrintLine("Region List End");
    OS::OSDebugPrintLine("");
  }

  void DX12DescriptorAllocator::PumpFreeQueue()
  {
#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugTitleBegin( "PumpFreeQueue" );
#endif

    int iiRegion{};
    int n{ mPendingFreeNodes.size() };
    while( iiRegion < n )
    {
      const RegionIndex iRegion{ mPendingFreeNodes[ iiRegion ] };
      dynmc RegionDesc* desc{ GetRegionAtIndex( iRegion ) };
      if( mCommandQueue->IsFenceComplete( desc->mFence ) )
      {
        // Change node from pending free to free

        // 1) remove from pending free list
        mPendingFreeNodes[ iiRegion ] = mPendingFreeNodes[ --n ];
        desc->mState = RegionDesc::kAllocated;

        // Resize during the loop instead of after because Free() calls DebugPrint() which 
        // prints mPendingFreeNodes
        mPendingFreeNodes.resize( n );

        Free( desc );
      }
      else
      {
        ++iiRegion;
      }
    }
    //mPendingFreeNodes.resize( n );

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugTitleEnd( "PumpFreeQueue" );
#endif
  }

  auto DX12DescriptorAllocator::GetRegionAtIndex( RegionIndex index ) -> RegionDesc*
  {
    return index == RegionIndex::kNull ? nullptr : &mRegions[ ( int )index ];
  }

  void DX12DescriptorAllocator::Free( RegionDesc* region )
  {
#if TAC_GPU_REGION_DEBUG()
    const RegionIndex index{ GetIndex( region ) };
    const String dbgTitle{  "Free( idx " + ToString( ( int )index ) + " )" };
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
    {
      if( (int)index == 2 )
        ++asdf;
      DebugTitleBegin( dbgTitle );
    }
#endif

    // was considered used till now (fence signalled)
    TAC_ASSERT( region->mDescriptorCount > 0 );
    TAC_ASSERT( region->mState == RegionDesc::kAllocated );

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
    else
    {
      region->mState = RegionDesc::kFree;
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

    TAC_ASSERT( region->mState == RegionDesc::kFree );

    if( !regionInFreeList )
      mFreeNodes.push_back( GetIndex( region ) );

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugPrint();

    for( RegionDesc* desc{ &mRegions[ 0 ] }; desc; desc = GetRegionAtIndex( desc->mRightIndex ) )
    {
      TAC_ASSERT( desc->mState != RegionDesc::kUnknown );
    }

    DebugTitleEnd( dbgTitle );
#endif
  }

  void DX12DescriptorAllocator::RemoveFromFreeList( RegionDesc* toRemove )
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

