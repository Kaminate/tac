#pragma once

#include "tac-win32/tac_win32_com_ptr.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{
  struct DX12DebugLayer;

  struct DX12InfoQueue
  {
    void Init( const DX12DebugLayer&, ID3D12Device*, Errors& );

    PCom< ID3D12InfoQueue >            m_infoQueue;
  };
}
