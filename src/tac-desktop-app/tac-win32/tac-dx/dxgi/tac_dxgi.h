#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h"

#include <dxgiformat.h> // DXGI_FORMAT
#include <dxgi1_6.h> // IDXGIObject, IDXGISwapChain4, IDXGIFactory4, IDXGIAdapter4

namespace Tac::Render
{
  void                  DXGIInit( Errors& );
  void                  DXGIUninit();
  DXGI_FORMAT           DXGIFormatFromTexFmt( TexFmt );
  DXGI_FORMAT           DXGIFormatFromVertexAttributeFormat( VertexAttributeFormat ); // todo: rename
  void                  DXGISetObjectName( IDXGIObject*, const StringView& );
  String                DXGIGetObjectName( IDXGIObject* );
  String                DXGICallAux( const char*, HRESULT );
  PCom< IDXGIAdapter4 > DXGIGetBestAdapter();
  void                  DXGICheckSwapEffect( DXGI_SWAP_EFFECT, Errors& );

  struct DXGISwapChainWrapper
  {
    struct Params
    {
      HWND        mHwnd        {};
      IUnknown*   mDevice      {}; // in dx12 this is a ID3D12CommandQueue
      int         mBufferCount {};
      int         mWidth       {};
      int         mHeight      {};
      DXGI_FORMAT mFmt         { DXGI_FORMAT_R16G16B16A16_FLOAT };
    };

    void             Init( Params, Errors& );
    void             Resize( v2i, Errors& );
    IDXGISwapChain4* GetIDXGISwapChain();
    IDXGISwapChain4* operator ->();

    operator bool();

  private:
    PCom< IDXGISwapChain4 > mDXGISwapChain;
    Params                  mParams;
  };


} // namespace Tac::Render

#define TAC_DXGI_CALL( call )                                                                      \
{                                                                                                  \
  const HRESULT hr { call };                                                                       \
  const bool failed { FAILED( hr ) };                                                              \
  TAC_RAISE_ERROR_IF( failed, Tac::Render::DXGICallAux( #call, hr ) );                             \
}

#define TAC_DXGI_CALL_RET( call )                                                                  \
{                                                                                                  \
  const HRESULT hr { call };                                                                       \
  const bool failed { FAILED( hr ) };                                                              \
  TAC_RAISE_ERROR_IF_RETURN( failed, Tac::Render::DXGICallAux( #call, hr ) );                      \
}

