#pragma once

#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-dx/dx12/resource/tac_dx12_resource.h"

#include <d3d12.h>

namespace Tac { struct Errors; }
namespace Tac::Render
{
  struct DX12TransitionHelper
  {
    struct Params
    {
      DX12Resource*          mResource    {};
      D3D12_RESOURCE_STATES  mStateAfter  {};
    };

    bool empty() const;
    void Append( Params );
    void ResourceBarrier( ID3D12GraphicsCommandList* );

  private:
    FixedVector< D3D12_RESOURCE_BARRIER, 10 > mFixedBarriers;
    Vector< D3D12_RESOURCE_BARRIER >          mOverflowBarriers;
  };
} // namespace Tac::Render

