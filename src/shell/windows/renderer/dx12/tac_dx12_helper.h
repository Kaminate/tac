#pragma once

#include "src/common/error/tac_error_handling.h"
#include "src/shell/windows/tac_win32_com_ptr.h"

#include <winnt.h> // HRESULT
#include <d3d12.h> // ID3D12Object*

namespace Tac::Render
{
  String      DX12CallAux( const char*, HRESULT );

  const char* DX12_HRESULT_ToString( HRESULT );

  void        DX12SetNameAux( ID3D12Object*, StringView );

  template< typename T >
  void DX12SetName( const PCom<T>& t, StringView sv ) { DX12SetNameAux( ( ID3D12Object* )t.Get(), sv ); }

} // namespace Tac::Render

#define TAC_DX12_CALL( call )                                                                      \
{                                                                                                  \
  const HRESULT hr = call;                                                                         \
  const bool failed = FAILED( hr );                                                                \
  TAC_RAISE_ERROR_IF( failed, Tac::Render::DX12CallAux( #call, hr ) );                             \
}

#define TAC_DX12_CALL_RET( ret, call )                                                             \
{                                                                                                  \
  const HRESULT hr = call;                                                                         \
  const bool failed = FAILED( hr );                                                                \
  TAC_RAISE_ERROR_IF_RETURN( failed, Tac::Render::DX12CallAux( #call, hr ), ret );                 \
}

