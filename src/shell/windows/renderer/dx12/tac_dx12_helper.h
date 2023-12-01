#pragma once

#include "src/common/core/tac_error_handling.h"

#include <winnt.h> // HRESULT
#include <d3d12.h> // ID3D12Object*

namespace Tac::Render
{
  void DX12CallAux( const char*, HRESULT, Errors& );

  void DX12SetName( ID3D12Object*, StringView );

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

