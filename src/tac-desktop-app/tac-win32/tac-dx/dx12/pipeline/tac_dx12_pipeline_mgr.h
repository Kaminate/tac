#pragma once

#include "tac-dx/dx12/pipeline/tac_dx12_pipeline.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac::Render { struct DX12ProgramMgr; struct DX12CommandQueue; }
namespace Tac::Render
{
  struct DX12PipelineMgr
  {

    PipelineHandle CreatePipeline( PipelineParams, Errors& );
    void           DestroyPipeline( PipelineHandle );
    DX12Pipeline*  FindPipeline( PipelineHandle );
    void           HotReload( Span< ProgramHandle > changed, Errors& );

  private:
    using DX12Pipelines = Array< DX12Pipeline, 100 >;

    void           CreatePipelineAtIndex( PipelineHandle, PipelineParams, Errors& );

    DX12Pipelines  mPipelines    {};
  };
} // namespace Tac::Render

