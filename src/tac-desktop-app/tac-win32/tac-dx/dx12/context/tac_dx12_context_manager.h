#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-dx/dx12/context/tac_dx12_context.h"
#include "tac-dx/dx12/tac_dx12_gpu_upload_allocator.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_backend.h"

#include <d3d12.h> // ID3D12...

namespace Tac::Render
{
  // a contextmanager manages contexts
  struct DX12ContextManager
  {
    void Init( Errors& );

    DX12Context*                      GetContext( Errors& );
    void                              RetireContext( DX12Context* );
    PCom< ID3D12GraphicsCommandList > CreateCommandList( Errors& );

  private:
    PCom< ID3D12Device4 >      mDevice            {};
    Vector< DX12Context* >     mAvailableContexts {};
    int                        mCommandListCount  {};
  };
}
