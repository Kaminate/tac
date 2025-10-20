#pragma once

#include <d3d12.h> // D3D12_DESCRIPTOR_HEAP_TYPE..., ID3D12Device*

namespace Tac::Render
{
  struct DX12DescriptorHeap;

  // Represents an allocation from a descriptor heap
  struct DX12Descriptor
  {
    auto GetCPUHandle( int = 0 ) const -> D3D12_CPU_DESCRIPTOR_HANDLE;
    auto GetGPUHandle( int = 0 ) const -> D3D12_GPU_DESCRIPTOR_HANDLE;
    bool IsValid() const;

    DX12DescriptorHeap* mOwner {};
    int                 mIndex {};
    int                 mCount {};
  };

} // namespace Tac::Render

