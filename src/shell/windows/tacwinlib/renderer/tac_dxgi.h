
#pragma once

#include "src/common/tac_error_handling.h"
#include "src/common/graphics/tac_renderer.h"

#include <dxgiformat.h> // DXGI_FORMAT
#include <dxgi.h> // IDXGIObject

#define TAC_RELEASE_IUNKNOWN( p ) { if( p ){ p->Release(); p = nullptr; } }

namespace Tac
{

    void           DXGIInit( Errors& );
    void           DXGIUninit();
    void           DXGICreateSwapChain( HWND hwnd,
                                    IUnknown* pDevice,
                                    int bufferCount,
                                    UINT width,
                                    UINT height,
                                    IDXGISwapChain** ppSwapChain,
                                    Errors& errors );


  DXGI_FORMAT      GetDXGIFormatTexture( Render::Format );
  DXGI_FORMAT      GetDXGIFormatTextureTypeless( int );
  DXGI_FORMAT      GetDXGIFormatDepth( int );

  void             NameDXGIObject( IDXGIObject* , StringView );

  void             DXGICallAux( const char* fnCallWithArgs, HRESULT , Errors& );

  const char*      TryInferDXGIErrorStr( HRESULT );

#define TAC_DXGI_CALL( errors, call, ... )\
{\
  HRESULT result = call( __VA_ARGS__ );\
  if( FAILED( result ) )\
  {\
    DXGICallAux( TAC_STRINGIFY( call ) "( " #__VA_ARGS__ " )", result, errors );\
    TAC_HANDLE_ERROR( errors );\
  }\
}


}

