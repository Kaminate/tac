#include "tac_example_dx12_1_window.h" // self-inc

#include "src/shell/tac_desktop_app.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/core/tac_error_handling.h"
#include "src/shell/windows/renderer/dxgi/tac_dxgi.h"
#include "src/shell/windows/renderer/dx12/tac_dx12_helper.h"

#include <d3d12.h> // D3D12...

#pragma comment( lib, "d3d12.lib" ) // D3D12...

namespace Tac
{
  static const int           bufferCount = 2;
  static DesktopWindowHandle hDesktopWindow;

  // ID3D12 objects
  static ID3D12Device*       sDevice;
  static ID3D12CommandQueue* sCommandQueue;
  static ID3D12CommandQueue* m_commandQueue;

  // IDXGI objects
  static IDXGISwapChain*     swapChain;

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

