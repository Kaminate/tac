#include "tac_dx12_transition_helper.h" // self-inc

namespace Tac::Render
{

  //void DX12TransitionHelper::Params::Execute( ID3D12GraphicsCommandList* commandList )
  //{
  //  DX12TransitionHelper helper;
  //  helper.Append( *this );
  //  helper.ResourceBarrier( commandList );
  //  mResource = {};
  //  mStateBefore = {};
  //  mStateAfter = {};
  //}

  bool DX12TransitionHelper::empty() const
  {
    return mFixedBarriers.empty() && mOverflowBarriers.empty();
  }

  void DX12TransitionHelper::Append( Params params )
  {

    DX12Resource* dx12Resource{ params.mResource };
    TAC_ASSERT( dx12Resource );

    const D3D12_RESOURCE_STATES StateBefore{ dx12Resource->GetState() };
    const D3D12_RESOURCE_STATES StateAfter{ params.mStateAfter };
    if( StateBefore == StateAfter )
      return;

    dx12Resource->SetState( StateAfter );

    ID3D12Resource* resource{ dx12Resource->Get() };

    const D3D12_RESOURCE_TRANSITION_BARRIER Transition
    {
      .pResource   { resource },
      .Subresource { D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES },
      .StateBefore { StateBefore },
      .StateAfter  { StateAfter },
    };

    const D3D12_RESOURCE_BARRIER barrier
    {
      .Type       { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION },
      .Flags      { D3D12_RESOURCE_BARRIER_FLAG_NONE },
      .Transition { Transition },
    };

    if( mFixedBarriers.size() < mFixedBarriers.max_size() )
    {
      mFixedBarriers.push_back( barrier );
    }
    else
    {
      mOverflowBarriers.push_back( barrier );
    }
  }

  void DX12TransitionHelper::ResourceBarrier( ID3D12GraphicsCommandList* commandList )
  {
    if( !mFixedBarriers.empty() )
    {
      commandList->ResourceBarrier( ( UINT )mFixedBarriers.size(), mFixedBarriers.data() );
      mFixedBarriers.clear();
    }

    if( !mOverflowBarriers.empty() )
    {
      commandList->ResourceBarrier( ( UINT )mOverflowBarriers.size(), mOverflowBarriers.data() );
      mOverflowBarriers.clear();
    }
  }
} // namespace Tac::Render

