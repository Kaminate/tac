#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"

#include <d3d12shader.h>
//struct D3D12_SHADER_INPUT_BIND_DESC;

namespace Tac::Render
{
  struct D3D12ProgramBinding
  {
    enum Type // DILIGENT SHADER_RESOURCE_TYPE
    {
      kUnknown = 0,

      kConstantBuffer,
      kSampler,
      kTextureUAV,
      kTextureSRV,
      kBufferUAV,
      kBufferSRV,
    };

    Type   mType = Type::kUnknown;
    String mName;
    int    mBindCount = -1; // A value of 0 represents an unbounded array
    int    mBindRegister = -1;
    int    mRegisterSpace = -1;
  };

  struct D3D12ProgramBindings : public Vector< D3D12ProgramBinding >
  {
    D3D12ProgramBindings() = default;
    D3D12ProgramBindings( const D3D12_SHADER_INPUT_BIND_DESC*, int );
  };

} // namespace Tac::Render
