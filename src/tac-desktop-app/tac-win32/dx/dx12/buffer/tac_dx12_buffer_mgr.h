#pragma once

#include "tac_dx12_dyn_buf.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render { struct DX12Context; struct DX12ContextManager; }
namespace Tac::Render
{
  struct DX12BufferMgr
  {
    struct Params
    {
      ID3D12Device*       mDevice{};
      DX12DescriptorHeap* mCpuDescriptorHeapCBV_SRV_UAV{};
      DX12ContextManager* mContextManager{};
    };

    void        Init( Params );
    void        CreateBuffer( BufferHandle, CreateBufferParams, Errors& );
    void        UpdateBuffer( BufferHandle, UpdateBufferParams, DX12Context*, Errors& );
    DX12Buffer* FindBuffer( BufferHandle );
    void        DestroyBuffer( BufferHandle );

  private:

    using DX12Buffers = Array< DX12Buffer, 100 >;

    struct DescriptorBindings
    {
      Optional< DX12Descriptor > mSRV;
      Optional< DX12Descriptor > mUAV;
    };

    DescriptorBindings  CreateBindings( ID3D12Resource* , CreateBufferParams );
    void                TransitionBuffer( Binding,
                                          ID3D12Resource*,
                                          D3D12_RESOURCE_STATES*,
                                          ID3D12GraphicsCommandList* );

    ID3D12Device*       mDevice                       {};
    DX12DescriptorHeap* mCpuDescriptorHeapCBV_SRV_UAV {};
    DX12ContextManager* mContextManager               {};
    DX12Buffers         mBuffers                      {};
  };
} // namespace Tac::Render
