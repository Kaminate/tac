#pragma once

#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h> // ID3D12...

namespace Tac::Render
{
  struct DX12BuiltInputLayout : public D3D12_INPUT_LAYOUT_DESC
  {
    DX12BuiltInputLayout( const VertexDeclarations& vtxDecls );
    
    FixedVector< D3D12_INPUT_ELEMENT_DESC, 10 > mElementDescs;
  };
}// namespace
