#pragma once

#include "src/shell/windows/tac_win32_com_ptr.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{

  struct DX12DebugLayer
  {
    void Init( Errors&);
    bool IsEnabled() const { return m_debugLayerEnabled; }

  private:
    bool m_debugLayerEnabled = false;
    PCom< ID3D12Debug > m_debug;
  };
  
  // Responsible for creating the device
  struct DX12DeviceInitializer
  {
    void Init( const DX12DebugLayer&, Errors&);

    PCom< ID3D12Device >              GetDevice()      { return m_device; }
    PCom< ID3D12DebugDevice >         GetDebugDevice() { return m_debugDevice; }
  private:

    PCom< ID3D12Device >              m_device;
    PCom< ID3D12DebugDevice >         m_debugDevice;
  };

  struct DX12InfoQueue
  {
    void Init(const DX12DebugLayer&,  ID3D12Device*, Errors& );
    PCom< ID3D12InfoQueue >            m_infoQueue;
  };

  bool DX12SupportsRayTracing( ID3D12Device* , Errors& );

  

} // namespace Tac::Render

