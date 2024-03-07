#include "tac_dx12_descriptor_heap.h" // self-inc
#include "tac_dx12_helper.h"

namespace Tac::Render
{
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

  D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::IndexGPUDescriptorHandle( int i ) const
  {
    TAC_ASSERT_INDEX( i, GetDescriptorCount() );
    return { mHeapStartGPU.ptr + i * mDescriptorSize };
  }

  D3D12_DESCRIPTOR_HEAP_TYPE DX12DescriptorHeap::GetType() const { return mDesc.Type; }

  void DX12DescriptorHeap::InitRTV( int n, ID3D12Device* device, Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
      .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
      .NumDescriptors = (UINT)n,
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

} // namespace Tac::Render

