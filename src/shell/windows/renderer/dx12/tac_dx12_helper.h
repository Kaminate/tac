#pragma once

#include "src/common/core/tac_error_handling.h"
#include "src/shell/windows/renderer/tac_dx.h"

#include <winnt.h> // HRESULT
#include <d3d12.h> // ID3D12Object*

namespace Tac::Render
{
  void DX12CallAux( const char*, HRESULT, Errors& );

  const char* DX12_HRESULT_ToString( HRESULT );

  void DX12SetNameAux( ID3D12Object*, StringView );

  template< typename T >
  void DX12SetName( const PCom<T>& t, StringView sv )
  {
    DX12SetNameAux( ( ID3D12Object* )t, sv );
  }

} // namespace Tac::Render

#define TAC_DX12_CALL( call, ... )                                                     \
{                                                                                      \
  const HRESULT result = call( __VA_ARGS__ );                                          \
  if( FAILED( result ) )                                                               \
  {                                                                                    \
    const char* fn = #call "(" #__VA_ARGS__ ")";                                       \
    TAC_CALL( Tac::Render::DX12CallAux, fn, result, errors );                          \
  }                                                                                    \
}

#define TAC_DX12_CALL_RET( ret, call, ... )                                            \
{                                                                                      \
  const HRESULT result = call( __VA_ARGS__ );                                          \
  if( FAILED( result ) )                                                               \
  {                                                                                    \
    const char* fn = #call "(" #__VA_ARGS__ ")";                                       \
    TAC_CALL_RET( ret, Tac::Render::DX12CallAux, fn, result, errors );                 \
  }                                                                                    \
}

