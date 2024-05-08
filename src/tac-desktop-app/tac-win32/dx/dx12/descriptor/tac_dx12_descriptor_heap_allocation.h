#pragma once

#include <d3d12.h> // D3D12_DESCRIPTOR_HEAP_TYPE..., ID3D12Device*

namespace Tac::Render
{
  struct DX12DescriptorHeap;
  struct DX12DescriptorHeapAllocation
  {
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const;
    bool Valid() const { return mIndex != -1; }

    DX12DescriptorHeap* mOwner { nullptr };
    int                 mIndex { -1 };
  };

} // namespace Tac::Render

