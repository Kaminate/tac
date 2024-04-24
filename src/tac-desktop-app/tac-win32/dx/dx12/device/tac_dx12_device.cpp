#include "tac_dx12_device.h" // self-inc
#include "tac_dx12_debug_layer.h"

#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dxgi/tac_dxgi.h"

namespace Tac::Render
{

  void DX12DeviceInitializer::Init( const DX12DebugLayer& debugLayer, Errors& errors )
  {
    TAC_ASSERT( !IsDebugMode || debugLayer.IsEnabled() );

    auto adapter { ( IDXGIAdapter* )DXGIGetBestAdapter() };
    TAC_DX12_CALL( D3D12CreateDevice(
                   adapter,
                   D3D_FEATURE_LEVEL_12_1,
                   m_device.iid(),
                   m_device.ppv() ) );
    DX12SetName( m_device, "Device" );

    if constexpr( IsDebugMode )
    {
      m_device.QueryInterface( m_debugDevice );
      TAC_ASSERT( m_debugDevice );
    }
  }

} // namespace Tac::Render

