#include "tac_dx12_descriptor_heap_allocation.h" // self-inc
#include "tac_dx12_descriptor_heap.h"

namespace Tac::Render
{
  D3D12_CPU_DESCRIPTOR_HANDLE DX12Descriptor::GetCPUHandle( int offset ) const
  {
    return mOwner->IndexCPUDescriptorHandle( mIndex + offset );
  }

  D3D12_GPU_DESCRIPTOR_HANDLE DX12Descriptor::GetGPUHandle( int offset ) const
  {
    return mOwner->IndexGPUDescriptorHandle( mIndex + offset);
  }

  bool DX12Descriptor::Valid() const
  {
    return mCount && mOwner;
  }

} // namespace Tac::Render

