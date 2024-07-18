#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string_view.h"

#include <d3d12.h> // ID3D12GraphicsCommandList

struct ID3D12GraphicsCommandList;

namespace Tac::Render
{
  struct PixRuntimeApi
  {
    static void Init( Errors& );
    static void BeginEvent( ID3D12GraphicsCommandList*, StringView );
    static void EndEvent( ID3D12GraphicsCommandList* );
    static void SetMarker( ID3D12GraphicsCommandList*, StringView );
  };
}

