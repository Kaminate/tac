#include "src/shell/windows/renderer/dx12/tac_dx12_enum_helper.h" // self-inc

#include "src/common/preprocess/tac_preprocessor.h"

namespace Tac::Render
{

  D3D12_TEXTURE_ADDRESS_MODE GetDX12AddressMode( AddressMode );
  D3D12_COMPARISON_FUNC      GetDX12Compare( Comparison );
  D3D12_FILTER               GetDX12Filter( Filter );
  UINT                       GetDX12CPUAccessFlags( CPUAccess );
  UINT                       GetDX12BindFlags( Binding );
  UINT                       GetDX12MiscFlags( Binding );
  D3D12_PRIMITIVE_TOPOLOGY   GetDX12PrimitiveTopology( PrimitiveTopology topology )
  {
    switch( topology )
    {
    case PrimitiveTopology::Unknown: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    case PrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    default: TAC_ASSERT_INVALID_CASE( topology ); return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
  }

  D3D12_COMPARISON_FUNC GetDX12DepthFunc( DepthFunc depthFunc )
  {
    switch( depthFunc )
    {
    case DepthFunc::Less: return D3D12_COMPARISON_FUNC_LESS;
    case DepthFunc::LessOrEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    default: TAC_ASSERT_INVALID_CASE( depthFunc ); return D3D12_COMPARISON_FUNC_LESS;
    }
  }

  D3D12_FILL_MODE GetDX12FillMode( FillMode fillMode )
  {
    switch( fillMode )
    {
    case FillMode::Solid: return D3D12_FILL_MODE_SOLID;
    case FillMode::Wireframe:return D3D12_FILL_MODE_WIREFRAME;
    default: TAC_ASSERT_INVALID_CASE( fillMode ); return D3D12_FILL_MODE_SOLID;
    }
  }

  D3D12_CULL_MODE GetDX12CullMode( CullMode cullMode )
  {
    switch( cullMode )
    {
    case CullMode::None: return D3D12_CULL_MODE_NONE;
    case CullMode::Back: return D3D12_CULL_MODE_BACK;
    case CullMode::Front: return D3D12_CULL_MODE_FRONT;
    default: TAC_ASSERT_INVALID_CASE( cullMode ); return D3D12_CULL_MODE_NONE;
    }
  }

  D3D12_BLEND GetDX12Blend( BlendConstants blendConstants )
  {
    switch( blendConstants )
    {
    case BlendConstants::One: return D3D12_BLEND_ONE;
    case BlendConstants::Zero: return D3D12_BLEND_ZERO;
    case BlendConstants::SrcRGB: return D3D12_BLEND_SRC_COLOR;
    case BlendConstants::SrcA: return D3D12_BLEND_SRC_ALPHA;
    case BlendConstants::OneMinusSrcA: return D3D12_BLEND_INV_SRC_ALPHA;
    default: TAC_ASSERT_INVALID_CASE( blendConstants ); return D3D12_BLEND_ONE;
    }
  }

  D3D12_BLEND_OP GetDX12BlendOp( BlendMode blendMode )
  {
    switch( blendMode )
    {
    case BlendMode::Add: return D3D12_BLEND_OP_ADD;
    default: TAC_ASSERT_INVALID_CASE( blendMode ); return D3D12_BLEND_OP_ADD;
    }
  }
} // namespace Tac::Render
