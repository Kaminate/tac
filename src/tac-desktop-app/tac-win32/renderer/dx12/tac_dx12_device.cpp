#include "tac_dx12_device.h" // self-inc
#include "tac_dx12_debug_layer.h"

#include "tac-win32/renderer/dx12/tac_dx12_helper.h"
#include "tac-win32/renderer/dxgi/tac_dxgi.h"

namespace Tac::Render
{

  void DX12Device::Init( const DX12DebugLayer& debugLayer, Errors& errors )
  {
    TAC_ASSERT( !IsDebugMode || debugLayer.IsEnabled() );

    auto adapter = ( IDXGIAdapter* )DXGIGetBestAdapter();
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

  ID3D12Device* DX12Device::GetID3D12Device() { return m_device.Get(); }

} // namespace Tac::Render

