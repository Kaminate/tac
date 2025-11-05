#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include <d3d12.h>

namespace Tac::Render
{
  struct DX12Resource
  {
    DX12Resource() = default;
    DX12Resource( PCom< ID3D12Resource >,
                  // v can be removed in favor of resource->getdesc?
                  D3D12_RESOURCE_DESC,
                  D3D12_RESOURCE_STATES );
    auto GetState() const -> D3D12_RESOURCE_STATES;
    void SetState( D3D12_RESOURCE_STATES );
    void LockState();
    void UnlockState();
    auto Get() -> ID3D12Resource*;
    auto operator->() -> ID3D12Resource*;
    operator ID3D12Resource*();

  private:
    D3D12_RESOURCE_STATES       mState       {};
    bool                        mStateLocked {};
    PCom< ID3D12Resource >      mResource    {};
    D3D12_RESOURCE_DESC         mDesc        {};
  };
} // namespace Tac::Render

