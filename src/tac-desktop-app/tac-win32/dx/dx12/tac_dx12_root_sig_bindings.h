#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"

#include <d3d12shader.h>
//struct D3D12_SHADER_INPUT_BIND_DESC;

namespace Tac::Render
{
  // this maybe should be renamed shaderbindings
  struct D3D12RootSigBinding
  {
    enum Type
    {
      kUnknown = 0,
      kCBuf,
      kSRV,
      kUAV,
      kSampler,
      kTexture,
    };

    Type   mType = Type::kUnknown;
    String mName;
    int    mBindCount = -1; // A value of 0 represents an unbounded array
    int    mBindRegister = -1;
    int    mRegisterSpace = -1;
  };

  struct D3D12RootSigBindings
  {
    D3D12RootSigBindings() = default;
    D3D12RootSigBindings( const D3D12_SHADER_INPUT_BIND_DESC*, int );

    Vector< D3D12RootSigBinding > mBindings;
  };

} // namespace Tac::Render
