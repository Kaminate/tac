#include "tac_dx12_info_queue.h" // self-inc
#include "tac_dx12_debug_layer.h"

#include "tac-dx/dx12/tac_dx12_helper.h"
#include "tac-std-lib/os/tac_os.h"

#if 0
extern "C" // https://devblogs.microsoft.com/directx/gettingstarted-dx12agility
{
  __declspec( dllexport ) extern const UINT  D3D12SDKVersion { 614 };
  __declspec( dllexport ) extern const char* D3D12SDKPath    { ".\\D3D12\\" };
}
#endif

namespace Tac::Render
{
  static const char* D3D12_MESSAGE_SEVERITY_ToString( D3D12_MESSAGE_SEVERITY Severity )
  {
    switch( Severity )
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION: return "Corruption";
    case D3D12_MESSAGE_SEVERITY_ERROR:      return "Error";
    case D3D12_MESSAGE_SEVERITY_WARNING:    return "Warning";
    case D3D12_MESSAGE_SEVERITY_INFO:       return "Info";
    case D3D12_MESSAGE_SEVERITY_MESSAGE:    return "Message";
    default:                                return "???";
    }
  }


  static void MyD3D12MessageFunc( D3D12_MESSAGE_CATEGORY Category,
                                  D3D12_MESSAGE_SEVERITY Severity,
                                  D3D12_MESSAGE_ID ID,
                                  LPCSTR pDescription,
                                  void* pContext )
  {
    TAC_UNUSED_PARAMETER( Category );
    TAC_UNUSED_PARAMETER( Severity );
    TAC_UNUSED_PARAMETER( ID );
    TAC_UNUSED_PARAMETER( pContext );
    if( pDescription )
      OS::OSDebugPrintLine(
        String()
        + D3D12_MESSAGE_SEVERITY_ToString( Severity )
        + ": "
        + pDescription );

    OS::OSDebugBreak();
  }

  void DX12InfoQueue::Init( const DX12DebugLayer& debugLayer,
                            ID3D12Device* device,
                            Errors& errors )
  {
    if constexpr( kIsDebugMode )
    {

      TAC_ASSERT( debugLayer.IsEnabled() );

      device->QueryInterface( mInfoQueue.iid(), mInfoQueue.ppv() );
      TAC_ASSERT( mInfoQueue );

      // Make the application debug break when bad things happen
      TAC_DX12_CALL( mInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE ) );
      TAC_DX12_CALL( mInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE ) );
      TAC_DX12_CALL( mInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE ) );

      // First available in Windows 10 Release Preview build 20236,
      // But as of 2023-12-11 not available on my machine :(
      if( PCom< ID3D12InfoQueue1 > infoQueue1{ mInfoQueue.QueryInterface< ID3D12InfoQueue1 >() } )
      {
        const D3D12MessageFunc             CallbackFunc{ MyD3D12MessageFunc };
        const D3D12_MESSAGE_CALLBACK_FLAGS CallbackFilterFlags{ D3D12_MESSAGE_CALLBACK_FLAG_NONE };
        void* pContext{ this };
        DWORD                              pCallbackCookie{};
        TAC_DX12_CALL( infoQueue1->RegisterMessageCallback( CallbackFunc,
                                                            CallbackFilterFlags,
                                                            pContext,
                                                            &pCallbackCookie ) );
      }
      else if( kIsDebugMode )
      {
        OS::OSDebugPrintLine( "ID3D12InfoQueue1 unavailable, sorry! (it is Windows 11 only)" );
      }
    }
  }
} // namespace Tac::Render
