#pragma once

#include "tac-win32/tac_win32_com_ptr.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{

  struct DX12ExampleDebugLayer
  {
    void Init( Errors& );
    bool IsEnabled() const;

  private:
    bool                mDebugLayerEnabled { false };
    PCom< ID3D12Debug > mDebug;
  };

  struct DX12ExampleDevice
  {
    // Responsible for creating the device
    void Init( const DX12ExampleDebugLayer&, Errors& );

    PCom< ID3D12Device >              mDevice;
    PCom< ID3D12DebugDevice >         mDebugDevice;
  };

  struct DX12ExampleInfoQueue
  {
    void Init( const DX12ExampleDebugLayer&, ID3D12Device*, Errors& );
    PCom< ID3D12InfoQueue >            mInfoQueue;
  };

  bool DX12SupportsRayTracing( ID3D12Device*, Errors& );



} // namespace Tac::Render

