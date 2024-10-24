#include "tac_dx12_resource.h"

#include "tac-std-lib/error/tac_assert.h"

namespace Tac::Render
{

  ctor                        DX12Resource::DX12Resource( PCom< ID3D12Resource > resource,
                                                          D3D12_RESOURCE_DESC desc,
                                                          D3D12_RESOURCE_STATES state )
    : mResource{ resource }
    , mDesc{ desc }
    , mState{ state }
  {
  }

  D3D12_RESOURCE_STATES       DX12Resource::GetState() const
  {
    return mState;
  }

  void                        DX12Resource::SetState( D3D12_RESOURCE_STATES state )
  {
    TAC_ASSERT( !mStateLocked );
    mState = state;
  }

  void                        DX12Resource::LockState()
  {
    TAC_ASSERT( !mStateLocked );
    mStateLocked = true;
  }

  void                        DX12Resource::UnlockState()
  {
    TAC_ASSERT( mStateLocked );
    mStateLocked = false;
  }

  ID3D12Resource*             DX12Resource::Get()
  {
    return mResource.Get();
  }

  ID3D12Resource*             DX12Resource::operator->()
  {
    return mResource.Get();
  }

  DX12Resource::operator ID3D12Resource*    ()
  {
    return mResource.Get();
  }
} // namespace Tac::Render

