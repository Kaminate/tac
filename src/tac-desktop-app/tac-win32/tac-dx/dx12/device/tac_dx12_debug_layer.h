#pragma once

#include "tac-win32/tac_win32_com_ptr.h"

#include <d3d12.h> // D3D12...

namespace Tac
{
  struct Errors;
}

namespace Tac::Render
{
  struct DX12DebugLayer
  {
    void Init( Errors& );
    bool IsEnabled() const { return m_debugLayerEnabled; }

  private:
    bool m_debugLayerEnabled { false };
    PCom< ID3D12Debug > m_debug;
  };

} // namespace Tac::Render

