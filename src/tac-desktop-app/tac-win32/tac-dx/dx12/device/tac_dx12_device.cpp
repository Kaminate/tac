#include "tac_dx12_device.h" // self-inc
#include "tac_dx12_debug_layer.h"

#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-dx/dxgi/tac_dxgi.h"

namespace Tac::Render
{

  void DX12DeviceInitializer::Init( const DX12DebugLayer& debugLayer, Errors& errors )
  {
    if constexpr( kIsDebugMode )
    {
      TAC_ASSERT( debugLayer.IsEnabled() );
    }

    if constexpr( kIsDebugMode ) // enable DRED before device creation
    {
      PCom< ID3D12DeviceRemovedExtendedDataSettings > dred;
      TAC_DX12_CALL( D3D12GetDebugInterface( dred.iid(), dred.ppv() ) );
      dred->SetAutoBreadcrumbsEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
      dred->SetPageFaultEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
    }

    IDXGIAdapter* adapter{ ( IDXGIAdapter* )DXGIGetBestAdapter() };
    TAC_DX12_CALL( D3D12CreateDevice(
                   adapter,
                   D3D_FEATURE_LEVEL_12_1,
                   mDevice.iid(),
                   mDevice.ppv() ) );

    ID3D12Device* pDevice{ mDevice.Get() };
    DX12SetName( pDevice, "Device" );

    if constexpr( kIsDebugMode )
    {
      mDevice.QueryInterface( mDebugDevice );
      TAC_ASSERT( mDebugDevice );
    }
  }

} // namespace Tac::Render

