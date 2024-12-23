#pragma once

#include "tac_dx12_dyn_buf.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render { struct DX12Context; }
namespace Tac::Render
{
  struct DX12BufferMgr
  {
    BufferHandle CreateBuffer( CreateBufferParams, Errors& );

    //           !?! eww
    void         UpdateBuffer( BufferHandle,
                               Span< const UpdateBufferParams >,
                               DX12Context*,
                               Errors& );

    DX12Buffer*  FindBuffer( BufferHandle );
    void         DestroyBuffer( BufferHandle );

  private:

    using DX12Buffers = Array< DX12Buffer, 100 >;

    struct DescriptorBindings
    {
      Optional< DX12Descriptor > mSRV;
      Optional< DX12Descriptor > mUAV;
    };

    DescriptorBindings  CreateBindings( ID3D12Resource* , CreateBufferParams );
    void                TransitionBuffer( Binding,
                                          DX12Resource*,
                                          ID3D12GraphicsCommandList* );

    DX12Buffers mBuffers{};
  };
} // namespace Tac::Render
