#include "tac_dx12_descriptor_heap.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_helper.h"

namespace Tac::Render
{



  // -----------------------------------------------------------------------------------------------
  void DX12DescriptorHeap::Init( const D3D12_DESCRIPTOR_HEAP_DESC& desc,
                                 ID3D12Device* device,
                                 Errors& errors )
  {
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors
    // Descriptors are the primary unit of binding for a single resource in D3D12.

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
    // A descriptor heap is a collection of contiguous allocations of descriptors,
    // one allocation for every descriptor.
    TAC_DX12_CALL( device->CreateDescriptorHeap(
      &desc,
      mHeap.iid(),
      mHeap.ppv() ) );
    mHeapStartCPU = mHeap->GetCPUDescriptorHandleForHeapStart();
    if( desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE )
      mHeapStartGPU = mHeap->GetGPUDescriptorHandleForHeapStart();
    mDescriptorSize = device->GetDescriptorHandleIncrementSize( desc.Type );
    mDesc = desc;
  }

  UINT DX12DescriptorHeap::GetDescriptorCount() const { return mDesc.NumDescriptors; }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::IndexCPUDescriptorHandle( int i ) const
  {
    TAC_ASSERT_INDEX( i, GetDescriptorCount() );
    return { mHeapStartCPU.ptr + i * mDescriptorSize };
  }

  // This function call is used for the following ID3D12GraphicsCommandList:: functions
  // ::ClearUnorderedAccessViewFloat
  // ::ClearUnorderedAccessViewUint
  // ::SetComputeRootDescriptorTable
  // ::SetGraphicsRootDescriptorTable
  D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::IndexGPUDescriptorHandle( int i ) const
  {
    TAC_ASSERT_INDEX( i, GetDescriptorCount() );
    TAC_ASSERT( mDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE );

    return { mHeapStartGPU.ptr + i * mDescriptorSize };
  }

  D3D12_DESCRIPTOR_HEAP_TYPE DX12DescriptorHeap::GetType() const { return mDesc.Type; }


  int                          DX12DescriptorHeap::AllocateIndex()
  {
    if( mFreeIndexes.empty() )
    {
      TAC_ASSERT( mUsedIndexCount < ( int )mDesc.NumDescriptors );
      return mUsedIndexCount++;
    }

    int i = mFreeIndexes.back();
    mFreeIndexes.pop_back();
    return i;
  }

  DX12DescriptorHeapAllocation DX12DescriptorHeap::Allocate()
  {
    return DX12DescriptorHeapAllocation
    {
      .mOwner = this,
      .mIndex = AllocateIndex(),
    };
  }

  void DX12DescriptorHeap::Free( DX12DescriptorHeapAllocation allocation )
  {
    TAC_ASSERT( allocation.mOwner == this );
    mFreeIndexes.push_back( allocation.mIndex );
  }

#if 0
  void DX12DescriptorHeap::InitRTV( int n, ID3D12Device* device, Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = (UINT)n,

      // D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE not allowed with D3D12_DESCRIPTOR_HEAP_TYPE_RTV
    };
    Init( desc, device, errors );
  }

  void DX12DescriptorHeap::InitSRV( int n, ID3D12Device* device, Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
      .NumDescriptors = ( UINT )n,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };

    Init( desc, device, errors );
  }

  void DX12DescriptorHeap::InitSampler( int n, ID3D12Device* device, Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
      .NumDescriptors = ( UINT )n,
      .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };

    Init( desc, device, errors );

  }
#endif

} // namespace Tac::Render
