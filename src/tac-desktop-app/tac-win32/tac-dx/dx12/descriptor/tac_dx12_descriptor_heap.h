// This file describes a DX12DescriptorHeap, which wraps ID3D12DescriptorHeap
#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac_dx12_descriptor_heap_allocation.h"

#include <d3d12.h> // D3D12_DESCRIPTOR_HEAP_TYPE..., ID3D12Device*

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12DescriptorAllocator;
  struct DX12CommandQueue;

  struct DX12DescriptorHeap
  {
    struct Params
    {
      D3D12_DESCRIPTOR_HEAP_DESC mHeapDesc     {};
      StringView                 mName         {};
    };
    dtor                         ~DX12DescriptorHeap();
    void                         Init( Params, Errors& );
    ID3D12DescriptorHeap*        GetID3D12DescriptorHeap();
    D3D12_DESCRIPTOR_HEAP_TYPE   GetType() const;
    UINT                         GetDescriptorCount() const;

    D3D12_CPU_DESCRIPTOR_HANDLE  IndexCPUDescriptorHandle( int ) const;
    D3D12_GPU_DESCRIPTOR_HANDLE  IndexGPUDescriptorHandle( int ) const;
    UINT                         GetDescriptorSize() const;

    DX12Descriptor               Allocate( int = 1 );
    void                         Free( DX12Descriptor );

    DX12DescriptorAllocator*     GetRegionMgr();
    StringView                   GetName();

  private:

    int                          AllocateIndex();

    String                       mName           {};
    DX12DescriptorAllocator*     mRegionMgr      {}; // owned
    int                          mUsedIndexCount {};
    Vector< int >                mFreeIndexes    {};
    PCom< ID3D12DescriptorHeap > mHeap           {};
    D3D12_CPU_DESCRIPTOR_HANDLE  mHeapStartCPU   {};
    D3D12_GPU_DESCRIPTOR_HANDLE  mHeapStartGPU   {};
    D3D12_DESCRIPTOR_HEAP_DESC   mDesc           {};
    UINT                         mDescriptorSize {};
  };

  struct DX12DescriptorHeapMgr
  {
    using HeapArray = Array< DX12DescriptorHeap, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES >;

    void Init( Errors& );
    void Bind( ID3D12GraphicsCommandList* );

    // CPU Heaps (used for creating resources)
    HeapArray mCPUHeaps;

    // GPU Heaps (used for rendering)
    HeapArray mGPUHeaps;

  private:
    void InitCPUHeaps( Errors& );
    void InitGPUHeaps( Errors& );
    void InitCPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE, int, StringView, Errors& );
    void InitGPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE, int, StringView, Errors& );
  };
} // namespace Tac::Render

