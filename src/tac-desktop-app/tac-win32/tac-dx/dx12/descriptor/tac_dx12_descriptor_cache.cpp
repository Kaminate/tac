#include "tac_dx12_descriptor_cache.h" // self-inc

namespace Tac::Render
{
  void DX12DescriptorCache::SetFence( FenceSignal fenceSignal )
  {
    for( DX12DescriptorRegion& gpuDesc : mGPUDescs )
      gpuDesc.Free( fenceSignal );

    mGPUDescs.clear();
  }

  void DX12DescriptorCache::AddDescriptorRegion( DX12DescriptorRegion&& region )
  {
    mGPUDescs.push_back( move( region ) );
  }

} // namespace Tac::Render
