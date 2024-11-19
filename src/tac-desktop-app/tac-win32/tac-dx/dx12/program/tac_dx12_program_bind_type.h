#pragma once

#include <d3d12shader.h> // D3D12_SHADER_INPUT_BIND_DESC

namespace Tac::Render
{
  // Represents the type of gpu resource that can be bound to a shader program
  struct D3D12ProgramBindType
  {
    enum Classification 
    {
      kUnknown = 0,

      kBufferSRV,
      kBufferUAV,
      kConstantBuffer,
      kSampler,
      kTextureSRV,
      kTextureUAV,
    };

    D3D12ProgramBindType() = default;
    D3D12ProgramBindType( const D3D12_SHADER_INPUT_BIND_DESC& );
    D3D12ProgramBindType( Classification );

    bool IsBuffer() const;
    bool IsConstantBuffer() const;
    bool IsSampler() const;
    bool IsTexture() const;
    bool IsValid() const;

    bool IsSRV() const;
    bool IsUAV() const;

    Classification GetClassification() const;

  private:
    static Classification Classify( const D3D12_SHADER_INPUT_BIND_DESC& );

    Classification mClassification {};
  };

} // namespace Tac::Render
