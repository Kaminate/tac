#include "tac_dx12_program_bindings.h" // self-inc

//#include <d3d12.h>



namespace Tac::Render
{

  // GetShaderResourceType() in DiligentEngine
  static D3D12ProgramBinding::Type ShaderInputToProgramBindType(
    const D3D12_SHADER_INPUT_BIND_DESC& info )
  {
    const D3D_SHADER_INPUT_TYPE Type { info.Type };
    switch( Type )
    {
    case D3D_SIT_CBUFFER:
      return D3D12ProgramBinding::Type::kConstantBuffer;

    case D3D_SIT_TEXTURE:
      return info.Dimension == D3D_SRV_DIMENSION_BUFFER
        ? D3D12ProgramBinding::Type::kBufferSRV
        : D3D12ProgramBinding::Type::kTextureSRV;

    case D3D_SIT_SAMPLER:
      return D3D12ProgramBinding::Type::kSampler;

    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
      return D3D12ProgramBinding::Type::kBufferSRV;

    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
      return D3D12ProgramBinding::Type::kBufferUAV;

    default: TAC_ASSERT_INVALID_CASE( Type );
      return D3D12ProgramBinding::Type::kUnknown;
    }
  }

  // -----------------------------------------------------------------------------------------------

  bool D3D12ProgramBinding::IsSampler() const { return mType == Type::kSampler; }

  bool D3D12ProgramBinding::IsBuffer() const
  {
    return
      mType == Type::kBufferUAV ||
      mType == Type::kBufferSRV ||
      mType == Type::kConstantBuffer;
  }


  bool D3D12ProgramBinding::IsTexture() const
  {
    return mType == Type::kTextureUAV || mType == Type::kTextureSRV;
  }

  bool D3D12ProgramBinding::IsFixedArray() const { return mBindCount > 1; }
  bool D3D12ProgramBinding::IsUnboundedArray() const { return mBindCount == 0; }
  bool D3D12ProgramBinding::IsArray() const { return mBindCount != 1; }
  bool D3D12ProgramBinding::IsSingleElement() const { return mBindCount == 1; }

  // -----------------------------------------------------------------------------------------------

  D3D12ProgramBindings::D3D12ProgramBindings( const D3D12_SHADER_INPUT_BIND_DESC* descs, int n )
  {
    for( int i{}; i < n; ++i )
    {
      const D3D12_SHADER_INPUT_BIND_DESC& info { descs[ i ] };
      const D3D12ProgramBinding::Type type { ShaderInputToProgramBindType( info ) };
      const D3D12ProgramBinding binding
      {
        .mType          { type },
        .mName          { info.Name },
        .mBindCount     { ( int )info.BindCount },
        .mBindRegister  { ( int )info.BindPoint },
        .mRegisterSpace { ( int )info.Space },
      };

      push_back( binding );
    }
  }


} // namespace Tac::Render
