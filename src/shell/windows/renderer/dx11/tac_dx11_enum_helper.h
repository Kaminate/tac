// This file implements the rendering backend using directx11

#pragma once

#include "src/common/graphics/tac_renderer.h"

#include <d3d11.h>

namespace Tac::Render
{
  D3D11_TEXTURE_ADDRESS_MODE GetAddressMode( AddressMode );
  D3D11_COMPARISON_FUNC      GetCompare( Comparison );
  D3D11_FILTER               GetFilter( Filter );
  D3D11_COMPARISON_FUNC      GetDepthFunc( DepthFunc );
  D3D11_USAGE                GetUsage( Access );
  UINT                       GetCPUAccessFlags( CPUAccess );
  UINT                       GetBindFlags( Binding );
  UINT                       GetMiscFlags( Binding );
  D3D11_BLEND                GetBlend( BlendConstants );
  D3D11_BLEND_OP             GetBlendOp( BlendMode );
  D3D11_FILL_MODE            GetFillMode( FillMode );
  D3D11_CULL_MODE            GetCullMode( CullMode );
  D3D11_PRIMITIVE_TOPOLOGY   GetPrimitiveTopology( PrimitiveTopology );
} // namespace Tac::Render

