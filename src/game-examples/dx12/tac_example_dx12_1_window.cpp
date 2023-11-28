#include "tac_example_dx12_1_window.h" // self-inc

#include "src/shell/tac_desktop_app.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/core/tac_error_handling.h"
#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"

#include <d3d12.h> // D3D12...
#include <dxgi1_6.h> // DXGI_CREATE_FACTORY_DEBUG

#pragma comment( lib, "d3d12.lib" ) // D3D12...


#define TAC_DX12_CALL( errors, call, ... )                                             \
{                                                                                      \
  const HRESULT result = call( __VA_ARGS__ );                                          \
  if( FAILED( result ) )                                                               \
  {                                                                                    \
    Tac::DX12CallAux( #call, #__VA_ARGS__, result, errors );                           \
    TAC_HANDLE_ERROR( errors );                                                        \
  }                                                                                    \
}

namespace Tac
{
  static ID3D12Device* sDevice;
  static ID3D12CommandQueue* sCommandQueue;

  static DesktopWindowHandle hDesktopWindow;

  static IDXGISwapChain* swapChain;
  static ID3D12CommandQueue* m_commandQueue;


  const int bufferCount = 2;
  

  static const char* DX12_HRESULT_ToString( const HRESULT hr )
  {
    switch( hr )
    {
    case D3D12_ERROR_ADAPTER_NOT_FOUND: return "D3D12_ERROR_ADAPTER_NOT_FOUND - The specified cached PSO was created on a different adapter and cannot be reused on the current adapter.";
    case D3D12_ERROR_DRIVER_VERSION_MISMATCH: return "D3D12_ERROR_DRIVER_VERSION_MISMATCH - The specified cached PSO was created on a different driver version and cannot be reused on the current adapter.";
    case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL - The method call is invalid.For example, a method's parameter may not be a valid pointer.";
    case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING - The previous blit operation that is transferring information to or from this surface is incomplete.";
    case E_FAIL: return "E_FAIL - Attempted to create a device with the debug layer enabled and the layer is not installed.";
    case E_INVALIDARG: return "E_INVALIDARG - An invalid parameter was passed to the returning function.";
    case E_OUTOFMEMORY: return "E_OUTOFMEMORY - Direct3D could not allocate sufficient memory to complete the call.";
    case E_NOTIMPL: return "E_NOTIMPL - The method call isn't implemented with the passed parameter combination.";
    case S_FALSE: return "S_FALSE - Alternate success value, indicating a successful but nonstandard completion( the precise meaning depends on context ).";
    case S_OK: return "S_OK - No error occurred.";
    default: TAC_ASSERT_INVALID_CASE( hr ); return "???";
    }
  }

  static void DX12CallAux( const char* fn, const char* args, HRESULT hr, Errors& errors )
  {
    String msg = fn;
    msg += "( ";
    msg += args;
    msg += " ) failed with ";
    msg += DX12_HRESULT_ToString( hr );

    errors.Append( msg );
  }

  void App::Init( Errors& errors )
  {
    if( IsDebugMode )
    {
      ID3D12Debug* dx12debug = nullptr;
      D3D12GetDebugInterface( IID_PPV_ARGS( &dx12debug ) );
      TAC_DX12_CALL( errors, D3D12GetDebugInterface,  IID_PPV_ARGS( &dx12debug )  );
      TAC_ON_DESTRUCT( dx12debug->Release() );

      dx12debug->EnableDebugLayer();
    }

    UINT dxgiFactoryFlags = IsDebugMode ? DXGI_CREATE_FACTORY_DEBUG : 0;

    DXGIInit(errors);
    TAC_HANDLE_ERROR( errors );

    IDXGIAdapter* adapter = DXGIGetAdapter();
    TAC_DX12_CALL( errors, D3D12CreateDevice, adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS( &sDevice ) );

    const D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    TAC_DX12_CALL( errors, sDevice->CreateCommandQueue, &queueDesc, IID_PPV_ARGS( &sCommandQueue ) );


    const DesktopAppCreateWindowParams desktopParams
    {
      .mName = "DX12 Window",
      .mX = 50,
      .mY = 50,
      .mWidth = 800,
      .mHeight = 600,
    };

    hDesktopWindow = DesktopAppCreateWindow( desktopParams );
    
      
    
  }

  void App::Update( Errors& errors )
  {
    if( !swapChain )
    {
      const DesktopWindowState* state = GetDesktopWindowState( hDesktopWindow );
      //const HWND hwnd = ( HWND )GetDesktopWindowNativeHandle( hDesktopWindow );
      const auto hwnd = ( HWND )state->mNativeWindowHandle;
      if( !hwnd )
        return;

      const auto width = ( UINT )state->mWidth;
      const auto height = ( UINT )state->mHeight;

      const D3D12_COMMAND_QUEUE_DESC queueDesc =
      {
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
      .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
      };

      TAC_DX12_CALL( errors, sDevice->CreateCommandQueue, &queueDesc, IID_PPV_ARGS( &m_commandQueue ) );

      DXGICreateSwapChain( hwnd,
                           m_commandQueue, // swap chain can force flush the queue
                           bufferCount,
                           width,
                           height,
                           &swapChain,
                           errors );
      TAC_HANDLE_ERROR( errors );

    }

    ++asdf;
  }

  void App::Uninit( Errors& )
  {
  }

  App App::sInstance
  {
    .mName = "DX12 Hello Window",
    .mDisableRenderer = true,
  };


} // namespace Tac

