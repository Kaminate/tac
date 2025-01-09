#pragma once

#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-dx/dx12/device/tac_dx12_debug_layer.h"

#include <d3d12.h> // D3D12...


namespace Tac::Render
{
  struct DX12InfoQueue
  {
    void Init( const DX12DebugLayer&, ID3D12Device*, Errors& );

    PCom< ID3D12InfoQueue >            mInfoQueue;
  };
}
