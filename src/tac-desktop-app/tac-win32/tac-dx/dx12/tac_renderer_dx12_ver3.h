#pragma once

#include "tac-std-lib/tac_ints.h" // u64
#include "tac-std-lib/containers/tac_array.h"
#include "tac-rhi/render3/tac_render_backend.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-dx/dxgi/tac_dxgi.h"
#include "tac-dx/dx12/program/tac_dx12_program_mgr.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_buffer_mgr.h"
#include "tac-dx/dx12/texture/tac_dx12_texture_mgr.h"
#include "tac-dx/dx12/buffer/tac_dx12_frame_buf_mgr.h"
#include "tac-dx/dx12/sampler/tac_dx12_sampler_mgr.h"
#include "tac-dx/dx12/device/tac_dx12_device.h"
#include "tac-dx/dx12/device/tac_dx12_info_queue.h"
#include "tac-dx/dx12/device/tac_dx12_debug_layer.h"
//#include "tac-dx/dx12/program/tac_dx12_program_bindings.h"
#include "tac-dx/dx12/tac_dx12_command_queue.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap.h"
#include "tac-dx/dx12/tac_dx12_command_allocator_pool.h"
#include "tac-dx/dx12/context/tac_dx12_context_manager.h"
#include "tac-dx/dx12/tac_dx12_gpu_upload_allocator.h"


#include <d3d12.h> // D3D12...

namespace Tac { struct Errors; }

namespace Tac::Render
{

  struct DX12Device : public IDevice
  {
    void Init( Errors& ) override;
    void Update( Errors& ) override;

    Info            GetInfo() const override;

    PipelineHandle  CreatePipeline( PipelineParams, Errors& ) override;
    IShaderVar*     GetShaderVariable( PipelineHandle, StringView ) override;
    void            DestroyPipeline( PipelineHandle ) override;

    ProgramHandle   CreateProgram( ProgramParams, Errors& ) override;
    String          GetProgramBindings_TEST( ProgramHandle ) override;
    void            DestroyProgram( ProgramHandle ) override;

    SamplerHandle   CreateSampler( CreateSamplerParams ) override;
    void            DestroySampler( SamplerHandle ) override;

    SwapChainHandle CreateSwapChain( SwapChainParams, Errors& ) override;
    void            ResizeSwapChain( SwapChainHandle, v2i, Errors& ) override;
    SwapChainParams GetSwapChainParams( SwapChainHandle ) override;
    void            DestroySwapChain( SwapChainHandle ) override;
    TextureHandle   GetSwapChainCurrentColor( SwapChainHandle ) override;
    TextureHandle   GetSwapChainDepth( SwapChainHandle ) override;
    void            Present( SwapChainHandle, Errors& ) override;

    BufferHandle    CreateBuffer( CreateBufferParams, Errors& ) override;
    void            DestroyBuffer( BufferHandle ) override;

    TextureHandle   CreateTexture( CreateTextureParams, Errors& ) override;
    void            DestroyTexture( TextureHandle ) override;

    IContext::Scope CreateRenderContext( Errors& ) override;
  };
} // namespace Tac::Render
