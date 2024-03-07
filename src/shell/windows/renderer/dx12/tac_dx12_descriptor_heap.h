// This file describes a DX12DescriptorHeap, which wraps ID3D12DescriptorHeap
#pragma once

#include "src/shell/windows/tac_win32_com_ptr.h" // PCom

#include <d3d12.h> // D3D12_DESCRIPTOR_HEAP_TYPE..., ID3D12Device*

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12DescriptorHeap
  {
    void Init( const D3D12_DESCRIPTOR_HEAP_DESC&, ID3D12Device*, Errors& );
    void InitRTV( int, ID3D12Device*, Errors& );
    void InitSRV( int, ID3D12Device*, Errors& );
    void InitSampler( int, ID3D12Device*, Errors& );

    D3D12_DESCRIPTOR_HEAP_TYPE  GetType() const;
    UINT                        GetDescriptorCount() const;

    D3D12_CPU_DESCRIPTOR_HANDLE IndexCPUDescriptorHandle( int ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE IndexGPUDescriptorHandle( int ) const;

  private:
    PCom< ID3D12DescriptorHeap > mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE  mHeapStartCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE  mHeapStartGPU;
    D3D12_DESCRIPTOR_HEAP_DESC   mDesc;
    UINT                         mDescriptorSize;
  };
} // namespace Tac::Render

