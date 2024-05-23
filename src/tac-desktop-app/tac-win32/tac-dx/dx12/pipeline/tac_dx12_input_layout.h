#pragma once

#include "tac-std-lib/containers/tac_fixed_vector.h"

#include <d3d12.h>

namespace Tac::Render { struct DX12Program; struct VertexDeclarations; }
namespace Tac::Render
{
  struct DX12InputLayout : public D3D12_INPUT_LAYOUT_DESC
  {
    DX12InputLayout( const VertexDeclarations&, const DX12Program* );

    FixedVector< D3D12_INPUT_ELEMENT_DESC, 10 > InputElementDescs;
  };

} // namespace Tac::Render

