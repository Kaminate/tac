#include "tac_dx12_descriptor_heap.h" // self-inc
#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dx12/tac_dx12_fence.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h"
#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"

#include "tac-dx/dx12/tac_dx12_command_queue.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  static const int kMaxCPUDescriptorsRTV        { 25 };
  static const int kMaxCPUDescriptorsDSV        { 25 };
  static const int kMaxCPUDescriptorsCBV_SRV_UAV{ 100 };
  static const int kMaxCPUDescriptorsSampler    { 20 };

  static const int kMaxGPUDescriptorsCBV_SRV_UAV{ 1000 };
  static const int kMaxGPUDescriptorsSampler    { 1000 };

  // -----------------------------------------------------------------------------------------------

  dtor                        DX12DescriptorHeap::~DX12DescriptorHeap()
  {
    TAC_DELETE mRegionMgr;
  }

  void                        DX12DescriptorHeap::Init( Params params, Errors& errors )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    const D3D12_DESCRIPTOR_HEAP_DESC& desc{ params.mHeapDesc };
    ID3D12Device* device{ renderer.mDevice };
    const StringView name{ params.mName };

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptors
    // Descriptors are the primary unit of binding for a single resource in D3D12.

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/descriptor-heaps
    // A descriptor heap is a collection of contiguous allocations of descriptors,
    // one allocation for every descriptor.
    TAC_DX12_CALL( device->CreateDescriptorHeap(
      &desc,
      mHeap.iid(),
      mHeap.ppv() ) );

    ID3D12DescriptorHeap* pHeap{ mHeap.Get() };
    DX12SetName( pHeap, name );

    mHeapStartCPU = pHeap->GetCPUDescriptorHandleForHeapStart();

    if( desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE )
      mHeapStartGPU = pHeap->GetGPUDescriptorHandleForHeapStart();

    mDescriptorSize = device->GetDescriptorHandleIncrementSize( desc.Type );
    mDesc = desc;
    mName = params.mName;

    const DX12DescriptorAllocator::Params regionMgrParams
    {
      .mDescriptorHeap { this },
    };

    mRegionMgr = TAC_NEW DX12DescriptorAllocator;
    mRegionMgr->Init( regionMgrParams );
  }

  UINT                        DX12DescriptorHeap::GetDescriptorCount() const
  {
    return mDesc.NumDescriptors;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorHeap::IndexCPUDescriptorHandle( int i ) const
  {
    TAC_ASSERT_INDEX( i, GetDescriptorCount() );
    return { mHeapStartCPU.ptr + i * mDescriptorSize };
  }

  UINT                        DX12DescriptorHeap::GetDescriptorSize() const
  {
    return mDescriptorSize;
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

  ID3D12DescriptorHeap*       DX12DescriptorHeap::GetID3D12DescriptorHeap()
  {
    return mHeap.Get();
  }

  D3D12_DESCRIPTOR_HEAP_TYPE  DX12DescriptorHeap::GetType() const
  {
    return mDesc.Type;
  }

  int                         DX12DescriptorHeap::AllocateIndex()
  {
    if( mFreeIndexes.empty() )
    {
      TAC_ASSERT( mUsedIndexCount < ( int )mDesc.NumDescriptors );
      return mUsedIndexCount++;
    }

    const int i { mFreeIndexes.back() };
    mFreeIndexes.pop_back();
    return i;
  }

  DX12Descriptor              DX12DescriptorHeap::Allocate( int n )
  {
    const int index{ AllocateIndex() };
    return DX12Descriptor
    {
      .mOwner { this },
      .mIndex { index },
      .mCount { 1 },
    };
  }

  void                        DX12DescriptorHeap::Free( DX12Descriptor allocation )
  {
    TAC_ASSERT( allocation.mCount == 1 );
    TAC_ASSERT( allocation.mOwner == this );
    mFreeIndexes.push_back( allocation.mIndex );
  }

  StringView                  DX12DescriptorHeap::GetName()
  {
    return mName;
  }

  DX12DescriptorAllocator*    DX12DescriptorHeap::GetRegionMgr()
  {
    return mRegionMgr;
  }

  // -----------------------------------------------------------------------------------------------

  void DX12DescriptorHeapMgr::InitCPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE type,
                                           int descriptorCount,
                                           StringView name,
                                           Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc
    {
      .Type           { type },
      .NumDescriptors { ( UINT )descriptorCount },
      .Flags          {},
      .NodeMask       {},
    };
    const DX12DescriptorHeap::Params params
    {
      .mHeapDesc     { desc },
      .mName         { name },
    };
    DX12DescriptorHeap& heap{ mCPUHeaps[ type ] };
    TAC_CALL( heap.Init( params, errors ) );
  }

  void DX12DescriptorHeapMgr::InitGPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE type,
                                           int descriptorCount,
                                           StringView name,
                                           Errors& errors )
  {
    const D3D12_DESCRIPTOR_HEAP_DESC desc
    {
      .Type           { type },
      .NumDescriptors { ( UINT )descriptorCount },
      .Flags          { D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE },
      .NodeMask       {},
    };
    const DX12DescriptorHeap::Params params
    {
      .mHeapDesc     { desc },
      .mName         { name },
    };
    DX12DescriptorHeap& heap{ mGPUHeaps[ type ] };
    TAC_CALL( heap.Init( params, errors ) );
  }

  void DX12DescriptorHeapMgr::InitCPUHeaps( Errors& errors )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    ID3D12Device* device{ renderer.mDevice };

    TAC_CALL( InitCPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                           kMaxCPUDescriptorsRTV,
                           "cpu rtv heap",
                           errors ) );

    TAC_CALL( InitCPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                           kMaxCPUDescriptorsDSV,
                           "cpu dsv heap",
                           errors ) );

    TAC_CALL( InitCPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                           kMaxCPUDescriptorsCBV_SRV_UAV,
                           "cpu cbv_srv_uav heap",
                           errors ) );

    TAC_CALL( InitCPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                           kMaxCPUDescriptorsSampler,
                           "cpu sampler heap",
                           errors ) );

  }

  void DX12DescriptorHeapMgr::InitGPUHeaps( Errors& errors )
  {
    DX12Renderer& renderer{ DX12Renderer::sRenderer };
    ID3D12Device* device{ renderer.mDevice };
    TAC_CALL( InitGPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                           kMaxGPUDescriptorsCBV_SRV_UAV,
                           "gpu cbv_srv_uav heap",
                           errors ) );

    TAC_CALL( InitGPUHeap( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                           kMaxGPUDescriptorsSampler,
                           "gpu sampler heap",
                           errors ) );
  }

  void DX12DescriptorHeapMgr::Init( Errors& errors )
  {
    TAC_CALL( InitCPUHeaps( errors ) );
    TAC_CALL( InitGPUHeaps( errors ) );
  }

  void DX12DescriptorHeapMgr::Bind( ID3D12GraphicsCommandList* commandList )
  {
    DX12DescriptorHeap& gpuResourceHeap{ mGPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ] };
    DX12DescriptorHeap& gpuSamplerHeap{ mGPUHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ] };
    ID3D12DescriptorHeap* gpuResourceHeapPtr{ gpuResourceHeap.GetID3D12DescriptorHeap() };
    ID3D12DescriptorHeap* gpuSamplerHeapPtr{ gpuSamplerHeap.GetID3D12DescriptorHeap() };

    const Array descHeaps
    {
      gpuResourceHeapPtr,
      gpuSamplerHeapPtr,
    };

    commandList->SetDescriptorHeaps( ( UINT )descHeaps.size(), descHeaps.data() );
  }


} // namespace Tac::Render

