#pragma once

#include "tac_dx12_dyn_buf.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"
//#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_allocator.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render { struct DX12Context; }
namespace Tac::Render
{
  struct DX12BufferMgr
  {
    auto CreateBuffer( CreateBufferParams, Errors& ) -> BufferHandle;

    //           !?! eww
    void UpdateBuffer( BufferHandle,
                       Span< const UpdateBufferParams >,
                       DX12Context*,
                       Errors& );

    auto FindBuffer( BufferHandle ) -> DX12Buffer*;
    void DestroyBuffer( BufferHandle );

  private:

    using DX12Buffers = Array< DX12Buffer, 1024 >;

    struct DescriptorBindings
    {
      Optional< DX12Descriptor > mSRV;
      Optional< DX12Descriptor > mUAV;
    };

    auto CreateDynamicBuffer( CreateBufferParams, Errors& ) -> BufferHandle;
    auto CreateNonDynamicBuffer( CreateBufferParams, Errors& ) -> BufferHandle;
    auto CreateBindings( ID3D12Resource* , CreateBufferParams ) -> DescriptorBindings;
    void TransitionBuffer( Binding, DX12Resource*, ID3D12GraphicsCommandList* );

    DX12Buffers mBuffers{};
  };
} // namespace Tac::Render
