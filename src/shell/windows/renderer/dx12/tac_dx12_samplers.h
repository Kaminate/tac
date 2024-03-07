#pragma once

#include <d3d12.h>

namespace Tac { struct Errors; }
namespace Tac::Render
{
  struct DX12DescriptorHeap;
  struct DX12Samplers
  {
    enum class Type
    {
      kPoint,
    };

    void Init( ID3D12Device*, DX12DescriptorHeap* );
  };
} // namespace Tac::Render

