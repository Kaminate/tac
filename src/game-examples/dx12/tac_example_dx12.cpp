#include "tac_example_dx12.h" // self-inc

#include "src/common/system/tac_os.h"
#include "src/common/string/tac_string.h"
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"
#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"

namespace Tac::Render
{


  static void MyD3D12MessageFunc( D3D12_MESSAGE_CATEGORY Category,
                                  D3D12_MESSAGE_SEVERITY Severity,
                                  D3D12_MESSAGE_ID ID,
                                  LPCSTR pDescription,
                                  void* pContext )
  {
    if( pDescription )
      OS::OSDebugPrintLine( pDescription );

    OS::OSDebugBreak();
  }

  // ----

  void DX12DebugLayer::Init( Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    TAC_DX12_CALL( D3D12GetDebugInterface( m_debug.iid(), m_debug.ppv() ) );

    // EnableDebugLayer must be called before the device is created
    m_debug->EnableDebugLayer();
    m_debugLayerEnabled = true;

    if( PCom< ID3D12Debug3 > debug3 = m_debug.QueryInterface<ID3D12Debug3>( ) )
    {

      // ( this should already be enabled by default )
      debug3->SetEnableSynchronizedCommandQueueValidation( TRUE );

      // https://learn.microsoft.com
      // GPU-based validation can be enabled only prior to creating a device. Disabled by default.
      //
      // https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation
      // GPU-based validation helps to identify the following errors:
      // - Use of uninitialized or incompatible descriptors in a shader.
      // - Use of descriptors referencing deleted Resources in a shader.
      // - Validation of promoted resource states and resource state decay.
      // - Indexing beyond the end of the descriptor heap in a shader.
      // - Shader accesses of resources in incompatible state.
      // - Use of uninitialized or incompatible Samplers in a shader.
      debug3->SetEnableGPUBasedValidation( TRUE );

    }

  }

  
  // ---

  void DX12DeviceInitializer::Init( const DX12DebugLayer& debugLayer , Errors& errors)
  {
    TAC_ASSERT( !IsDebugMode || debugLayer.IsEnabled() );

    auto adapter = ( IDXGIAdapter* )Tac::Render::DXGIGetBestAdapter();
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


  void DX12InfoQueue::Init( const DX12DebugLayer& debugLayer,
                            ID3D12Device* device,
                            Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    TAC_ASSERT( debugLayer.IsEnabled() );

    device->QueryInterface( m_infoQueue.iid(), m_infoQueue.ppv() );
    TAC_ASSERT(m_infoQueue);

    // Make the application debug break when bad things happen
    TAC_DX12_CALL(m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE ) );
    TAC_DX12_CALL(m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE ) );
    TAC_DX12_CALL(m_infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE ) );

    // First available in Windows 10 Release Preview build 20236,
    // But as of 2023-12-11 not available on my machine :(
    if( auto infoQueue1 = m_infoQueue.QueryInterface<ID3D12InfoQueue1>() )
    {
      const D3D12MessageFunc CallbackFunc = MyD3D12MessageFunc;
      const D3D12_MESSAGE_CALLBACK_FLAGS CallbackFilterFlags = D3D12_MESSAGE_CALLBACK_FLAG_NONE;
      void* pContext = this;
      DWORD pCallbackCookie = 0;

      TAC_DX12_CALL( infoQueue1->RegisterMessageCallback(
                     CallbackFunc,
                     CallbackFilterFlags,
                     pContext,
                     &pCallbackCookie ) );
    }
  }

  bool DX12SupportsRayTracing( ID3D12Device* device, Errors& errors )
  {
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 opt5{};
    TAC_DX12_CALL_RET( false,
                       device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS5,
                       &opt5,
                       sizeof( opt5 ) ) );
    return opt5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
  }

} // namespace Tac


