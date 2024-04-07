#include "tac_dx12_descriptor_heap_allocation.h" // self-inc
#include "tac_dx12_descriptor_heap.h"

namespace Tac::Render
{
  D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeapAllocation::GetCPUHandle()
  {
    return mOwner->IndexCPUDescriptorHandle( mIndex );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeapAllocation::GetGPUHandle()
  {
    return mOwner->IndexGPUDescriptorHandle( mIndex );
  }


} // namespace Tac::Render

