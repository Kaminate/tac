#pragma once

#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h>

namespace Tac::Render
{
  D3D12_TEXTURE_ADDRESS_MODE GetDX12AddressMode( AddressMode );
  D3D12_COMPARISON_FUNC      GetDX12Compare( Comparison );
  D3D12_FILTER               GetDX12Filter( Filter );
  D3D12_COMPARISON_FUNC      GetDX12DepthFunc( DepthFunc );
  UINT                       GetDX12CPUAccessFlags( CPUAccess );
  UINT                       GetDX12BindFlags( Binding );
  UINT                       GetDX12MiscFlags( Binding );
  D3D12_BLEND                GetDX12Blend( BlendConstants );
  D3D12_BLEND_OP             GetDX12BlendOp( BlendMode );
  D3D12_FILL_MODE            GetDX12FillMode( FillMode );
  D3D12_CULL_MODE            GetDX12CullMode( CullMode );
  D3D12_PRIMITIVE_TOPOLOGY   GetDX12PrimitiveTopology( PrimitiveTopology );
} // namespace Tac::Render

