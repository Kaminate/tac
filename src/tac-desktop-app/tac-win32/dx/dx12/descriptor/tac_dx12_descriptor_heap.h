// This file describes a DX12DescriptorHeap, which wraps ID3D12DescriptorHeap
#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac_dx12_descriptor_heap_allocation.h"

#include <d3d12.h> // D3D12_DESCRIPTOR_HEAP_TYPE..., ID3D12Device*

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12DescriptorHeap
  {

    void Init( const D3D12_DESCRIPTOR_HEAP_DESC&, ID3D12Device*, Errors& );
#if 0
    void InitRTV( int, ID3D12Device*, Errors& );
    void InitSRV( int, ID3D12Device*, Errors& );
    void InitSampler( int, ID3D12Device*, Errors& );
#endif

    D3D12_DESCRIPTOR_HEAP_TYPE   GetType() const;
    UINT                         GetDescriptorCount() const;

    D3D12_CPU_DESCRIPTOR_HANDLE  IndexCPUDescriptorHandle( int ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE  IndexGPUDescriptorHandle( int ) const;

    DX12DescriptorHeapAllocation Allocate();
    void                         Free( DX12DescriptorHeapAllocation );

  private:
    int                          AllocateIndex();

    int                          mUsedIndexCount = 0;
    Vector< int >                mFreeIndexes;
    PCom< ID3D12DescriptorHeap > mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE  mHeapStartCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE  mHeapStartGPU;
    D3D12_DESCRIPTOR_HEAP_DESC   mDesc;
    UINT                         mDescriptorSize;
  };
} // namespace Tac::Render
