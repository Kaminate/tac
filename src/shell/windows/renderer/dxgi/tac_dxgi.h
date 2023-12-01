
#pragma once

#include "src/common/core/tac_error_handling.h"
#include "src/common/graphics/tac_renderer.h"

#include <dxgiformat.h> // DXGI_FORMAT
#include <dxgi1_5.h> // IDXGIObject, IDXGISwapChain4

#define TAC_RELEASE_IUNKNOWN( p ) { if( p ){ p->Release(); p = nullptr; } }

namespace Tac
{

  void             DXGIInit( Errors& );
  void             DXGIUninit();
  IDXGISwapChain4* DXGICreateSwapChain( HWND hwnd,
                                        IUnknown* pDevice,
                                        int bufferCount,
                                        UINT width,
                                        UINT height,
                                        Errors& errors );


  DXGI_FORMAT      GetDXGIFormatTexture( Render::Format );
  DXGI_FORMAT      GetDXGIFormatTextureTypeless( int );
  DXGI_FORMAT      GetDXGIFormatDepth( int );

  void             DXGISetObjectName( IDXGIObject*, const StringView& );
  String           DXGIGetObjectName( IDXGIObject* );

  void             DXGICallAux( const char*, HRESULT, Errors& );

  const char*      TryInferDXGIErrorStr( HRESULT );


  //               represents a display subsystem (GPU, VRAM, etc)
  IDXGIAdapter*    DXGIGetAdapter();


} // namespace Tac

#define TAC_DXGI_CALL( call, ... )                                                              \
{                                                                                               \
  HRESULT result = call( __VA_ARGS__ );                                                         \
  if( FAILED( result ) )                                                                        \
  {                                                                                             \
    TAC_CALL( Tac::DXGICallAux, TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )", result, errors ); \
  }                                                                                             \
}
