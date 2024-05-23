#pragma once

#include "tac-dx/dx12/pipeline/tac_dx12_pipeline.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac::Render { struct DX12ProgramMgr; }
namespace Tac::Render
{
  struct DX12PipelineMgr
  {
    void          Init( ID3D12Device*, DX12ProgramMgr* );
    void          CreatePipeline( PipelineHandle, PipelineParams, Errors& );
    void          DestroyPipeline( PipelineHandle );
    DX12Pipeline* FindPipeline( PipelineHandle );

  private:
    using DX12Pipelines = Array< DX12Pipeline, 100 >;

    DX12Pipelines   mPipelines  {};
    ID3D12Device*   mDevice     {};
    DX12ProgramMgr* mProgramMgr {};
  };
} // namespace Tac::Render

