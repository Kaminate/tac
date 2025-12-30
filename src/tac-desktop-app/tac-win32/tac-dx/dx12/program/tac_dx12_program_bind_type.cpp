#include "tac_dx12_program_bind_type.h" // self-inc
#include "tac-std-lib/error/tac_assert.h"


namespace Tac::Render
{

  static auto Classify( const D3D12_SHADER_INPUT_BIND_DESC& info ) -> D3D12ProgramBindType::Classification
  {
    const D3D_SHADER_INPUT_TYPE Type{ info.Type };
    switch( Type )
    {
    case D3D_SIT_CBUFFER: return D3D12ProgramBindType::kConstantBuffer;

    case D3D_SIT_TEXTURE: return info.Dimension == D3D_SRV_DIMENSION_BUFFER
      ? D3D12ProgramBindType::kBufferSRV
      : D3D12ProgramBindType::kTextureSRV;

    case D3D_SIT_SAMPLER: return D3D12ProgramBindType::kSampler;

    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
      return D3D12ProgramBindType::kBufferSRV;

    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
      return info.Dimension == D3D_SRV_DIMENSION_BUFFER
        ? D3D12ProgramBindType::kBufferUAV
        : D3D12ProgramBindType::kTextureUAV;

    default: TAC_ASSERT_INVALID_CASE( Type ); return D3D12ProgramBindType::kUnknown;
    }
  }

  // GetShaderResourceType() in DiligentEngine
  D3D12ProgramBindType::D3D12ProgramBindType( const D3D12_SHADER_INPUT_BIND_DESC& info )
    : mClassification{ Classify( info ) }
  {
  }

  D3D12ProgramBindType::D3D12ProgramBindType( Classification classification )
    : mClassification{ classification }
  {
  }


  bool D3D12ProgramBindType::IsValid() const
  {
    return mClassification != kUnknown;
  }

  bool D3D12ProgramBindType::IsSampler() const
  {
    return mClassification == kSampler;
  }

  bool D3D12ProgramBindType::IsBuffer()  const
  {
    return
      mClassification == kBufferUAV ||
      mClassification == kBufferSRV ||
      mClassification == kConstantBuffer;
  }

  bool D3D12ProgramBindType::IsTexture()  const
  {
    return mClassification == kTextureUAV || mClassification == kTextureSRV;
  }

  bool D3D12ProgramBindType::IsSRV()  const
  {
    return mClassification == kTextureSRV || mClassification == kBufferSRV;
  }

  bool D3D12ProgramBindType::IsUAV() const
  {
    return mClassification == kTextureUAV || mClassification == kBufferUAV;
  }

  bool D3D12ProgramBindType::IsConstantBuffer()  const
  {
    return mClassification == kConstantBuffer;
  }

  auto D3D12ProgramBindType::GetClassification() const -> Classification { return mClassification; }

  auto D3D12ProgramBindType::ToString() -> const char*
  {
    switch( mClassification )
    {
      case kUnknown: return "kUnknown";
      case kBufferSRV: return "kBufferSRV";
      case kBufferUAV: return "kBufferUAV";
      case kConstantBuffer: return "kConstantBuffer";
      case kSampler: return "kSampler";
      case kTextureSRV: return "kTextureSRV";
      case kTextureUAV: return "kTextureUAV";
      default: TAC_ASSERT_INVALID_CASE( mClassification ); return "???";
    }
  }

} // namespace Tac::Render
