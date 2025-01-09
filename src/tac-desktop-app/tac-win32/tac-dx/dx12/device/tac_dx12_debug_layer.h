#pragma once

#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-std-lib/error/tac_error_handling.h"

#include <d3d12.h> // D3D12...

namespace Tac::Render
{
  struct DX12DebugLayer
  {
    void Init( Errors& );
    bool IsEnabled() const;

  private:
    bool                mDebugLayerEnabled {};
    PCom< ID3D12Debug > mDebug             {};
  };

} // namespace Tac::Render

