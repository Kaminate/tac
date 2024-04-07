#include "tac_dx12_program_bindings.h" // self-inc

//#include <d3d12.h>



namespace Tac::Render
{

  static D3D12ProgramBinding::Type ShaderInputToProgramBindType( D3D_SHADER_INPUT_TYPE Type )
  {
    switch( Type )
    {
    case D3D_SIT_CBUFFER: return D3D12ProgramBinding::Type::kCBuf;
    case D3D_SIT_TEXTURE: return D3D12ProgramBinding::Type::kTexture;
    case D3D_SIT_SAMPLER: return D3D12ProgramBinding::Type::kSampler;
    case D3D_SIT_BYTEADDRESS: return D3D12ProgramBinding::Type::kSRV;
    default: TAC_ASSERT_INVALID_CASE( Type ); return D3D12ProgramBinding::Type::kUnknown;
    }
  }

  D3D12ProgramBindings::D3D12ProgramBindings( const D3D12_SHADER_INPUT_BIND_DESC* descs, int n )
  {
    for( int i = 0; i < n; ++i )
    {
      const D3D12_SHADER_INPUT_BIND_DESC& info = descs[ i ];
      D3D12ProgramBinding::Type type = ShaderInputToProgramBindType( info.Type );
      D3D12ProgramBinding binding
      {
        .mType = type,
        .mName = info.Name,
        .mBindCount = ( int )info.BindCount,
        .mBindRegister = ( int )info.BindPoint,
        .mRegisterSpace = ( int )info.Space,
      };

      mBindings.push_back( binding );
    }
  }
} // namespace Tac::Render
