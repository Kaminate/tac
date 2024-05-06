#include "tac_dx12_debug_layer.h" // self-inc
   
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
//#include <d3d12sdklayers.h>

namespace Tac::Render
{
  void DX12DebugLayer::Init( Errors& errors )
  {
    if constexpr( !IsDebugMode )
      return;

    TAC_DX12_CALL( D3D12GetDebugInterface( m_debug.iid(), m_debug.ppv() ) );

    // EnableDebugLayer must be called before the device is created
    m_debug->EnableDebugLayer();
    m_debugLayerEnabled = true;

    if( PCom< ID3D12Debug3 > debug3{ m_debug.QueryInterface< ID3D12Debug3 >() } )
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

    if( PCom< ID3D12Debug5 > debug5{ m_debug.QueryInterface< ID3D12Debug5 >() } )
    {
      debug5->SetEnableAutoName( TRUE );
    }


  }

} // namespace Tac::Render

