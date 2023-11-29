#pragma once

#include "src/common/core/tac_error_handling.h"

#include <winnt.h> // HRESULT

namespace Tac::Render
{
  void DX12CallAux( const char*, const char*, HRESULT, Errors& );
} // namespace Tac::Render

#define TAC_DX12_CALL( errors, call, ... )                                             \
{                                                                                      \
  const HRESULT result = call( __VA_ARGS__ );                                          \
  if( FAILED( result ) )                                                               \
  {                                                                                    \
    Tac::Render::DX12CallAux( #call, #__VA_ARGS__, result, errors );                   \
    TAC_HANDLE_ERROR( errors );                                                        \
  }                                                                                    \
}

