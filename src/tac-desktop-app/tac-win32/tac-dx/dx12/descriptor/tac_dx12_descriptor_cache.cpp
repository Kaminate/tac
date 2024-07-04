#include "tac_dx12_descriptor_cache.h" // self-inc

namespace Tac::Render
{

  void DX12DescriptorCache::SetFence( FenceSignal fenceSignal )
  {
    for( DX12DescriptorRegion& gpuDesc : mGPUDescs )
      gpuDesc.SetFence( fenceSignal );

    Clear();
  }

  void DX12DescriptorCache::Clear()
  {
    mCPUDescs.clear();
    mGPUDescs.clear();
    mGPUIndexes.clear();
  }

  void DX12DescriptorCache::SetRegionManager( DX12DescriptorRegionManager* gpuRegionMgr )
  {
    mGpuRegionMgr = gpuRegionMgr;
  }

  DX12DescriptorRegion* DX12DescriptorCache::GetGPUDescriptorForCPUDescriptors(
    Span< DX12Descriptor > cpuDescriptors )
  {
    const int nDescriptors{ cpuDescriptors.size() };

    if( nDescriptors == 1 )
      if( DX12DescriptorRegion * gpuDescriptor{ Lookup( cpuDescriptors[ 0 ] ) } )
        return gpuDescriptor;

    if( nDescriptors == 1 )
    {
      const int iDesc{ mGPUDescs.size() };
      mCPUDescs.push_back( cpuDescriptors[ 0 ] );
      mGPUIndexes.push_back( iDesc );
    }

    mGPUDescs.push_back( move( mGpuRegionMgr->Alloc( nDescriptors ) ) );

    DX12DescriptorRegion* result{ &mGPUDescs.back() };
    TAC_ASSERT( result->IsValid() );
    return result;
  }

  DX12DescriptorRegion* DX12DescriptorCache::Lookup( DX12Descriptor cpuDescriptor )
  {
    const int n{ mCPUDescs.size() };
    for( int i{}; i < n; ++i )
    {
      if( mCPUDescs[ i ].mIndex == cpuDescriptor.mIndex )
      {
        TAC_ASSERT( mCPUDescs[ i ].mOwner == cpuDescriptor.mOwner );
        TAC_ASSERT( mCPUDescs[ i ].mCount >= cpuDescriptor.mCount );
        return &mGPUDescs[ mGPUIndexes[ i ] ];
      }
    }
    return nullptr;
  }

} // namespace Tac::Render
