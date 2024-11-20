#pragma once

namespace Tac::Render
{
  struct CommitParams
  {
    ID3D12GraphicsCommandList* mCommandList        {};
    bool                       mIsCompute          {};
    UINT                       mRootParameterIndex {};
  };

} // namespace Tac::Render

