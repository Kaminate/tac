#pragma once

#include "src/common/error/tac_error_handling.h"
#include "src/common/graphics/renderer/tac_renderer.h"
#include "src/shell/windows/tac_win32_com_ptr.h"
#include "src/shell/windows/tac_win32_com_ptr.h"

#include <dxgiformat.h> // DXGI_FORMAT
#include <dxgi1_6.h> // IDXGIObject, IDXGISwapChain4, IDXGIFactory4, IDXGIAdapter4

namespace Tac::Render
{

  void             DXGIInit( Errors& );
  void             DXGIUninit();

  struct SwapChainCreateInfo
  {
    HWND      mHwnd = nullptr;
    IUnknown* mDevice = nullptr; // in dx12 this is a ID3D12CommandQueue
    int       mBufferCount = 0;
    int       mWidth = 0;
    int       mHeight = 0;
  };

  PCom<IDXGISwapChain4> DXGICreateSwapChain( const SwapChainCreateInfo&, Errors& );


  DXGI_FORMAT      DXGIGetSwapChainFormat();
  DXGI_FORMAT      GetDXGIFormatTexture( Format );
  DXGI_FORMAT      GetDXGIFormatTextureTypeless( int );
  DXGI_FORMAT      GetDXGIFormatDepth( int );

  void             DXGISetObjectName( IDXGIObject*, const StringView& );
  String           DXGIGetObjectName( IDXGIObject* );

  String           DXGICallAux( const char*, HRESULT );

  //               represents a display subsystem (GPU, VRAM, etc)
  PCom<IDXGIAdapter4>    DXGIGetBestAdapter();

  void CheckSwapEffect(DXGI_SWAP_EFFECT, Errors&);

} // namespace Tac::Render

#define TAC_DXGI_CALL( call )                                                                      \
{                                                                                                  \
  const HRESULT hr = call;                                                                         \
  const bool failed = FAILED( hr );                                                                \
  TAC_RAISE_ERROR_IF( failed, Tac::Render::DXGICallAux( #call, hr ) );                             \
}

#define TAC_DXGI_CALL_RET( ret, call )                                                             \
{                                                                                                  \
  const HRESULT hr = call;                                                                         \
  const bool failed = FAILED( hr );                                                                \
  TAC_RAISE_ERROR_IF_RETURN( failed, Tac::Render::DXGICallAux( #call, hr ), ret );                 \
}
