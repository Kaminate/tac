#pragma once

#include "tac-win32/dx/dx12/pipeline/tac_dx12_pipeline.h"
#include "tac-rhi/render3/tac_render_api.h"

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
    DX12Pipeline    mPipelines[ 100 ];
    ID3D12Device*   mDevice{};
    DX12ProgramMgr* mProgramMgr{};
  };
} // namespace Tac::Render

