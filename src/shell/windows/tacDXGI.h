
#pragma once

#include "src/common/tacErrorHandling.h"
#include "src/common/graphics/tacRenderer.h"

#include <dxgi1_6.h> // IDXGIFactory4, IDXGIAdapter4

#define TAC_RELEASE_IUNKNOWN( p ) { if( p ){ p->Release(); p = nullptr; } }

namespace Tac
{

  struct DXGI
  {
    ~DXGI();
    void           Init( Errors& );
    void           Uninit();
    void           CreateSwapChain( HWND hwnd,
                                    IUnknown* pDevice,
                                    int bufferCount,
                                    UINT width,
                                    UINT height,
                                    IDXGISwapChain** ppSwapChain,
                                    Errors& errors );
    IDXGIFactory4* mFactory = nullptr;
    IDXGIAdapter4* mDxgiAdapter4 = nullptr;
  };


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

