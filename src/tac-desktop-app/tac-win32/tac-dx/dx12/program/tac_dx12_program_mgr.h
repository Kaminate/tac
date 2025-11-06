#pragma once

#include "tac-dx/dx12/program/tac_dx12_program.h"
#include "tac-dx/dx12/pipeline/tac_dx12_pipeline_mgr.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac::Render
{
  struct DX12ProgramMgr
  {
    void Init( Errors& );
    auto CreateProgram( ProgramParams, Errors& ) -> ProgramHandle;
    auto GetProgramBindings_TEST( ProgramHandle ) -> String;
    void DestroyProgram( ProgramHandle );
    auto FindProgram( ProgramHandle ) -> DX12Program*;
    void HotReload( Errors& );

  private:

    void CreateProgramAtIndex( ProgramHandle, ProgramParams, Errors& );

    Array< DX12Program, 100 > mPrograms    {};
    ID3D12Device*             mDevice      {};
    DX12PipelineMgr*          mPipelineMgr {};
  };
} // namespace Tac::Render

