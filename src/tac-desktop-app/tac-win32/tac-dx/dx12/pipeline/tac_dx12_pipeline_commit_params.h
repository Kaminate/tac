#pragma once

#include <d3d12.h>

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_cache.h"

namespace Tac::Render
{
  struct CommitParams
  {
    ID3D12GraphicsCommandList* mCommandList        {};
    DX12DescriptorCache*       mDescriptorCache    {};
    bool                       mIsCompute          {};
    UINT                       mRootParameterIndex {};
  };

} // namespace Tac::Render

