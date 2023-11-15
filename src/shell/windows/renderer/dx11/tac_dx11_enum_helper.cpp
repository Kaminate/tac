#include "src/shell/windows/renderer/dx11/tac_dx11_enum_helper.h" // self-inc


namespace Tac::Render
{
  D3D11_TEXTURE_ADDRESS_MODE GetAddressMode( const AddressMode addressMode )
  {
    switch( addressMode )
    {
      case AddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
      case AddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
      case AddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
      default: TAC_ASSERT_INVALID_CASE( addressMode ); return D3D11_TEXTURE_ADDRESS_WRAP;
    }
  }

  D3D11_COMPARISON_FUNC      GetCompare( const Comparison compare )
  {
    switch( compare )
    {
      case Comparison::Always: return D3D11_COMPARISON_ALWAYS;
      case Comparison::Never: return D3D11_COMPARISON_NEVER;
      default: TAC_ASSERT_INVALID_CASE( compare ); return D3D11_COMPARISON_ALWAYS;
    }
  };

  D3D11_FILTER               GetFilter( const Filter filter )
  {
    switch( filter )
    {
      case Filter::Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      case Filter::Point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
      case Filter::Aniso: return D3D11_FILTER_ANISOTROPIC;
      default: TAC_ASSERT_INVALID_CASE( filter ); return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }
  };

  D3D11_COMPARISON_FUNC      GetDepthFunc( const DepthFunc depthFunc )
  {
    switch( depthFunc )
    {
      case DepthFunc::Less: return D3D11_COMPARISON_LESS;
      case DepthFunc::LessOrEqual: return D3D11_COMPARISON_LESS_EQUAL;
      default: TAC_ASSERT_INVALID_CASE( depthFunc ); return D3D11_COMPARISON_LESS;
    }
  }

  D3D11_USAGE                GetUsage( const Access access )
  {
    switch( access )
    {
      case Access::Default: return D3D11_USAGE_DEFAULT;
      case Access::Dynamic: return D3D11_USAGE_DYNAMIC;
      case Access::Static: return D3D11_USAGE_IMMUTABLE;
      default: TAC_ASSERT_INVALID_CASE( access ); return D3D11_USAGE_DEFAULT;
    }
  }

  UINT                       GetCPUAccessFlags( const CPUAccess access )
  {
    return 0
      | ( ( int )access & ( int )CPUAccess::Read ? D3D11_CPU_ACCESS_READ : 0 )
      | ( ( int )access & ( int )CPUAccess::Write ? D3D11_CPU_ACCESS_WRITE : 0 );
  }

  UINT                       GetBindFlags( const Binding binding )
  {
    return 0
      | ( ( int )( binding & Binding::RenderTarget ) ? D3D11_BIND_RENDER_TARGET : 0 )
      | ( ( int )( binding & Binding::ShaderResource ) ? D3D11_BIND_SHADER_RESOURCE : 0 )
      | ( ( int )( binding & Binding::DepthStencil ) ? D3D11_BIND_DEPTH_STENCIL : 0 )
      | ( ( int )( binding & Binding::UnorderedAccess ) ? D3D11_BIND_UNORDERED_ACCESS : 0 );
  }

  UINT                       GetMiscFlags( const Binding binding )
  {
    if( ( int )binding & ( int )Binding::RenderTarget &&
      ( int )binding & ( int )Binding::ShaderResource )
      return D3D11_RESOURCE_MISC_GENERATE_MIPS;
    return 0;
  }

  D3D11_BLEND                GetBlend( const BlendConstants blendConstants )
  {
    switch( blendConstants )
    {
      case BlendConstants::OneMinusSrcA: return D3D11_BLEND_INV_SRC_ALPHA;
      case BlendConstants::SrcA:         return D3D11_BLEND_SRC_ALPHA;
      case BlendConstants::SrcRGB:       return D3D11_BLEND_SRC_COLOR;
      case BlendConstants::Zero:         return D3D11_BLEND_ZERO;
      case BlendConstants::One:          return D3D11_BLEND_ONE;
      default: TAC_ASSERT( false );      return D3D11_BLEND_ZERO;
    }
  };

  D3D11_BLEND_OP             GetBlendOp( const BlendMode mode )
  {
    switch( mode )
    {
      case BlendMode::Add:          return D3D11_BLEND_OP_ADD;
      default: TAC_ASSERT( false ); return D3D11_BLEND_OP_ADD;
    }
  };

  D3D11_FILL_MODE            GetFillMode( const FillMode fillMode )
  {
    switch( fillMode )
    {
      case FillMode::Solid:     return D3D11_FILL_SOLID;
      case FillMode::Wireframe: return D3D11_FILL_WIREFRAME;
      default: TAC_ASSERT_INVALID_CASE( fillMode ); return ( D3D11_FILL_MODE )0;
    }
  }

  D3D11_CULL_MODE            GetCullMode( const CullMode cullMode )
  {
    switch( cullMode )
    {
      case CullMode::None:  return D3D11_CULL_NONE;
      case CullMode::Back:  return D3D11_CULL_BACK;
      case CullMode::Front: return D3D11_CULL_FRONT;
      default: TAC_ASSERT_INVALID_CASE( cullMode ); return ( D3D11_CULL_MODE )0;
    }
  }

  D3D11_PRIMITIVE_TOPOLOGY   GetPrimitiveTopology( const PrimitiveTopology primitiveTopology )
  {
    switch( primitiveTopology )
    {
      case PrimitiveTopology::PointList:    return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
      case PrimitiveTopology::TriangleList: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
      case PrimitiveTopology::LineList:     return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
      default: TAC_ASSERT_INVALID_CASE( primitiveTopology );
    }
    return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  }

} // namespace Tac::Render

