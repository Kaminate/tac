#pragma once

#include "src/common/error/tac_error_handling.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/shell/windows/renderer/tac_dx.h"

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

  void             DXGICallAux( const char*, HRESULT, Errors& );



  //               represents a display subsystem (GPU, VRAM, etc)
  PCom<IDXGIAdapter4>    DXGIGetBestAdapter();


} // namespace Tac::Render

#define TAC_DXGI_CALL( call, ... )                                       \
{                                                                        \
  const HRESULT result = call( __VA_ARGS__ );                            \
  if( FAILED( result ) )                                                 \
  {                                                                      \
    const char* fn = TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )";       \
    TAC_CALL( Tac::Render::DXGICallAux, fn, result, errors );            \
  }                                                                      \
}

#define TAC_DXGI_CALL_RET( ret, call, ... )                              \
{                                                                        \
  const HRESULT result = call( __VA_ARGS__ );                            \
  if( FAILED( result ) )                                                 \
  {                                                                      \
    const char* fn = TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )";       \
    TAC_CALL_RET( ret, Tac::Render::DXGICallAux, fn, result, errors );   \
  }                                                                      \
}
