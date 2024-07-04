#pragma once

#include <d3d12.h> // D3D12_DESCRIPTOR_HEAP_TYPE..., ID3D12Device*

namespace Tac::Render
{
  struct DX12DescriptorHeap;

  // Represents an allocation from a descriptor heap, which can hold a CPU descriptor handle
  struct DX12Descriptor
  {
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle( int = 0 ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle( int = 0 ) const;
    bool IsValid() const;

    DX12DescriptorHeap* mOwner {};
    int                 mIndex {};
    int                 mCount {};
  };

} // namespace Tac::Render

