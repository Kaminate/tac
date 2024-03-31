#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac::Render
{
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
    int    mCount = 0; // 0 == unbounded
  };

  struct D3D12RootSigBindings
  {
    Vector< D3D12RootSigBinding > mBindings;
  };

} // namespace Tac::Render
