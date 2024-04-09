#include "tac_dx12_samplers.h" // self-inc
#if 0

#include "tac_dx12_descriptor_heap.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac::Render
{

  static void CreateSampler( D3D12_SAMPLER_DESC desc,
                             DX12Samplers::Type type,
                             ID3D12Device* device,
                             DX12DescriptorHeap* heap )
  {
    TAC_ASSERT( heap->GetType() == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER );
    const D3D12_CPU_DESCRIPTOR_HANDLE dst = heap->IndexCPUDescriptorHandle( ( int )type );
    device->CreateSampler( &desc, dst );
  }

  void DX12Samplers::Init( ID3D12Device* device, DX12DescriptorHeap* heap )
  {
    const D3D12_SAMPLER_DESC pointDesc
    {
      .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
      .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
      .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
      .MinLOD = 0,
      .MaxLOD = D3D12_FLOAT32_MAX,
    };
    CreateSampler( pointDesc, Type::kPoint, device, heap );
  }

} // namespace Tac::Render

#endif
