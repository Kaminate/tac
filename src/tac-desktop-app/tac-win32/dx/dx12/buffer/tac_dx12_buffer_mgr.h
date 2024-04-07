#pragma once

#include "tac_dx12_dyn_buf.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12BufferMgr
  {
    void Init( ID3D12Device* );
    void CreateDynBuf( DynBufHandle, int, StackFrame, Errors& );
    void UpdateDynBuf( RenderApi::UpdateDynBufParams );
    void DestroyDynBuf( DynBufHandle);

    DX12DynBuf    mDynBufs[ 100 ];
    ID3D12Device* mDevice{};
  };
} // namespace Tac::Render
