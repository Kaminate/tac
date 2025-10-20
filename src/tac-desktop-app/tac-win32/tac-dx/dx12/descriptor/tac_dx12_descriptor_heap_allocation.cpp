#include "tac_dx12_descriptor_heap_allocation.h" // self-inc
#include "tac_dx12_descriptor_heap.h"

namespace Tac::Render
{
  auto DX12Descriptor::GetCPUHandle( int offset ) const -> D3D12_CPU_DESCRIPTOR_HANDLE
  {
    return mOwner->IndexCPUDescriptorHandle( mIndex + offset );
  }

  auto DX12Descriptor::GetGPUHandle( int offset ) const -> D3D12_GPU_DESCRIPTOR_HANDLE
  {
    return mOwner->IndexGPUDescriptorHandle( mIndex + offset);
  }

  bool DX12Descriptor::IsValid() const
  {
    return mCount && mOwner;
  }

} // namespace Tac::Render

