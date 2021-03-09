
#pragma once

#include "src/common/tacErrorHandling.h"
#include "src/common/graphics/tacRenderer.h"

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


  Format GetFormat( DXGI_FORMAT format );
  DXGI_FORMAT GetDXGIFormat( Format textureFormat );

  void NameDXGIObject( IDXGIObject* object, StringView name );

  void DXGICallAux( const char* fnCallWithArgs, HRESULT res, Errors& errors );

  const char* TryInferDXGIErrorStr( HRESULT );

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

