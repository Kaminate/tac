#include "tac_dx12_descriptor_heap_gpu_mgr.h" // self-inc
#include "tac-std-lib/os/tac_os.h"

#define TAC_GPU_REGION_DEBUG() 0
#if TAC_GPU_REGION_DEBUG()
#define TAC_GPU_REGION_DEBUG_TYPE D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
#endif

namespace Tac::Render
{

  StringView DX12DescriptorRegionManager::RegionDesc::StateToString( State state )
  {
    switch( state )
    {
    case State::kUnknown: return "unknown";
    case State::kAllocated: return "allocated";
    case State::kFree: return "free";
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

    void DX12DescriptorRegion::SetFence( FenceSignal fenceSignal )
    {

      DX12DescriptorRegionManager::RegionDesc* regionDesc{
        mRegionManager->GetRegionAtIndex( mRegionIndex ) };

      TAC_ASSERT( regionDesc->mState == DX12DescriptorRegionManager::RegionDesc::kAllocated );
      regionDesc->mState = DX12DescriptorRegionManager::RegionDesc::kPendingFree;
      mRegionManager->mPendingFreeNodes.push_back( mRegionManager->GetIndex( regionDesc ) );
      mRegionManager = nullptr;
      mRegionIndex = DX12DescriptorRegionManager::RegionIndex::kNull;
    }

  // -----------------------------------------------------------------------------------------------

  //DX12DescriptorRegionManager::RegionDesc* DX12DescriptorRegionManager::RegionDesc::GetLeft( DX12DescriptorRegionManager* mgr )
  //{
  //  return mgr->GetRegionAtIndex( mLeftIndex );
  //}

  //DX12DescriptorRegionManager::RegionDesc* DX12DescriptorRegionManager::RegionDesc::GetRight( DX12DescriptorRegionManager* mgr )
  //{
  //  return mgr->GetRegionAtIndex( mRightIndex );
  //}

  void DX12DescriptorRegionManager::RegionDesc::PosInsertAfter( RegionDesc* left, DX12DescriptorRegionManager* mgr )
  {
    const RegionIndex iMiddle{ mgr->GetIndex( this ) };
    const RegionIndex iRight{ left->mRightIndex };
    const RegionIndex iLeft{ mgr->GetIndex( left ) };

    left->mRightIndex = iMiddle;
    if( RegionDesc * right{ mgr->GetRegionAtIndex( iRight ) } )
      right->mLeftIndex = iMiddle;

    mLeftIndex = iLeft;
    mRightIndex = iRight;
  }

  void DX12DescriptorRegionManager::RegionDesc::PosRemove(  DX12DescriptorRegionManager* mgr )
  {
    if( RegionDesc * left{ mgr->GetRegionAtIndex( mLeftIndex ) } )
      left->mRightIndex = mRightIndex;

    if( RegionDesc * right{ mgr->GetRegionAtIndex( mRightIndex ) } )
      right->mLeftIndex = mRightIndex;

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
      .mLeftIndex       { RegionIndex::kNull   },
      .mRightIndex      { RegionIndex::kNull },
      .mDescriptorIndex { 0 },
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
      OS::OSDebugPrintLine( "alloc " + ToString( n ) + ( n == 1 ? " byte" : " bytes") );
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
      return {};

    if( allocRegion->mDescriptorCount > descriptorCount )
    {
      const RegionIndex iExtra{ mRegions.size() };
      mRegions.resize( ( int )iExtra + 1 );
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

    const DX12Descriptor descriptor
    {
      .mOwner { mOwner },
      .mIndex { allocRegion->mDescriptorIndex },
      .mCount { allocRegion->mDescriptorCount },
    };
    return DX12DescriptorRegion( descriptor, this, iAlloc );
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
    str += mOwner->GetName() + ": ";
    str += DebugFreeListString();

    int loopCount {};
    String regionSeparator{""};
    for( RegionDesc* desc{ &mRegions[ 0 ] }; desc; desc = GetRegionAtIndex( desc->mRightIndex ) )
    {
      ++loopCount;
      TAC_ASSERT( loopCount < ( int )mOwner->GetDescriptorCount() );
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

      str += regionSeparator + "[" + properties.mString + "]";
      regionSeparator = ", ";
    }

    OS::OSDebugPrintLine(str);
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
  }

//  void DX12DescriptorRegionManager::Free( DX12Descriptor descriptor, FenceSignal fenceSignal )
//  {
//    RegionDesc* region{ GetRegion( descriptor ) };
//
//#if TAC_GPU_REGION_DEBUG()
//    for( RegionToFree regionToFree : mRegionsToFree )
//    {
//      TAC_ASSERT( regionToFree.mRegion != region );
//    }
//#endif
//
//    const RegionToFree regionToFree
//    {
//      .mRegion { region },
//      .mFence  { fenceSignal },
//    };
//    mRegionsToFree.push_back( regionToFree );
//  }


  DX12DescriptorRegionManager::RegionDesc* DX12DescriptorRegionManager::GetRegionAtIndex( RegionIndex index )
  {
    return index == RegionIndex::kNull ? nullptr : &mRegions[ ( int )index ];
  }

  //DX12DescriptorRegionManager::RegionDesc* DX12DescriptorRegionManager::GetRegion(
  //  DX12Descriptor descriptor )
  //{
  //  TAC_ASSERT( descriptor.Valid() );
  //  TAC_ASSERT( descriptor.mOwner == mOwner );
  //  TAC_ASSERT( descriptor.mCount > 0 );
  //  TAC_ASSERT_INDEX( descriptor.mIndex, mAllRegions.size() );

  //  RegionDesc* region{ &mAllRegions[ descriptor.mIndex ] };

  //  TAC_ASSERT( region->mSize == descriptor.mCount );
  //  TAC_ASSERT( region->mLeft );
  //  TAC_ASSERT( region->mRight );

  //  return region;
  //}

  //void DX12DescriptorRegionManager::FreeNoSignal( DX12Descriptor descriptor )
  //{
  //  RegionDesc* region{ GetRegion( descriptor ) };
  //  Free( region );
  //}

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
    TAC_ASSERT( region->mDescriptorCount > 0 );
    TAC_ASSERT( region->mLeftIndex != RegionIndex::kNull );
    TAC_ASSERT( region->mRightIndex != RegionIndex::kNull );

    bool regionInFreeList{ false };
    if( RegionDesc* left{ GetRegionAtIndex( region->mLeftIndex ) };
        left && left->mState == RegionDesc::kFree )
    {
      left->mDescriptorCount += region->mDescriptorCount;

      *region = {};
      mUnusedNodes.push_back( GetIndex( region ) );

      region = left;

      // Since we merged into left, which was in the free list, we are now in the free list
      regionInFreeList = true;
    }

    if( RegionDesc* right{ GetRegionAtIndex( region->mRightIndex ) };
        right && right->mState == RegionDesc::kFree )
    {
      region->mDescriptorCount += right->mDescriptorCount;
      right->PosRemove( this );
      *right = {};
      mUnusedNodes.push_back( region->mRightIndex );

      RemoveFromFreeList( right );

    }

    if( !regionInFreeList )
      mFreeNodes.push_back( GetIndex( region ) );

#if TAC_GPU_REGION_DEBUG()
    if( mOwner->GetType() == TAC_GPU_REGION_DEBUG_TYPE )
      DebugPrint();
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

