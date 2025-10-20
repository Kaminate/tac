#pragma once

#include "tac-std-lib/error/tac_error_handling.h" // TAC_RAISE_ERROR_IF
#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-rhi/render3/tac_render_handle.h"

#include <winnt.h> // HRESULT
#include <d3d12.h> // ID3D12Object*

namespace Tac::Render
{
  auto DX12CallAux( const char*, HRESULT ) -> String;
  auto DX12_HRESULT_ToString( HRESULT ) -> const char*;

  struct DX12NameHelper
  {
    void NameObject( ID3D12Object* ) const;

    StringView     mName          {};
    StackFrame     mStackFrame    {};
    ResourceHandle mHandle        {};
  };

  auto DX12GetName( ID3D12Object* ) -> StringView;
  void DX12SetName( ID3D12Object*, StringView );

  template< typename T >
  void DX12SetName( const PCom< T >& t, StringView sv ) { DX12SetName( t.Get(), sv ); }
} // namespace Tac::Render

#define TAC_DX12_CALL( call )                                                                      \
{                                                                                                  \
  const HRESULT hr { call };                                                                       \
  const bool failed { FAILED( hr ) };                                                              \
  TAC_RAISE_ERROR_IF( failed, Tac::Render::DX12CallAux( #call, hr ) );                             \
}

#define TAC_DX12_CALL_RET( call )                                                                  \
{                                                                                                  \
  const HRESULT hr { call };                                                                       \
  const bool failed { FAILED( hr ) };                                                              \
  TAC_RAISE_ERROR_IF_RETURN( failed, Tac::Render::DX12CallAux( #call, hr ) );                      \
}

