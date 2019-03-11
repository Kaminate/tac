#pragma once

#include "common/tacErrorHandling.h"
#include "common/graphics/tacRenderer.h"

#include <dxgi1_6.h> // IDXGIFactory4, IDXGIAdapter4

#define TAC_RELEASE_IUNKNOWN( p ) { if( p ){ p->Release(); p = nullptr; } }

struct TacDXGI
{
  ~TacDXGI()
  {
    TAC_RELEASE_IUNKNOWN( mFactory );
    TAC_RELEASE_IUNKNOWN( mDxgiAdapter4 );
  }
  void Init( TacErrors& errors );
  void CreateSwapChain(
    HWND hwnd,
    IUnknown* pDevice,
    int bufferCount,
    UINT width,
    UINT height,
    IDXGISwapChain** ppSwapChain,
    TacErrors& errors );
  
  IDXGIFactory4* mFactory = nullptr;
  IDXGIAdapter4* mDxgiAdapter4 = nullptr;
 };


TacFormat GetTacFormat( DXGI_FORMAT format );
DXGI_FORMAT GetDXGIFormat( TacFormat textureFormat );

void TacNameDXGIObject( IDXGIObject* object, const TacString& name );

void TacDXGICallAux( const char* fnCallWithArgs, HRESULT res, TacErrors& errors );

#define TAC_DXGI_CALL( errors, call, ... )\
{\
  HRESULT result = call( __VA_ARGS__ );\
  if( FAILED( result ) )\
  {\
    TacDXGICallAux( TacStringify( call ) "( " #__VA_ARGS__ " )", result, errors );\
    TAC_HANDLE_ERROR( errors );\
  }\
}

