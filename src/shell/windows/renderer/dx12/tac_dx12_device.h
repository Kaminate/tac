#pragma once

#include "src/shell/windows/tac_win32_com_ptr.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{
  struct DX12DebugLayer;

  struct DX12Device
  {
    // Creates the device
    void Init( const DX12DebugLayer&, Errors& );
    ID3D12Device* GetID3D12Device();

    PCom< ID3D12Device >              m_device;
    PCom< ID3D12DebugDevice >         m_debugDevice;
  };

} // namespace Tac::Render
