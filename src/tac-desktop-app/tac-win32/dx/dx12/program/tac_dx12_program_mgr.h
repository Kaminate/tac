#pragma once

#include "tac-win32/dx/dx12/program/tac_dx12_program.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  struct DX12ProgramMgr
  {
    void         Init( ID3D12Device*, Errors& );
    void         CreateProgram( ProgramHandle, ProgramParams, Errors& );
    void         DestroyProgram(ProgramHandle);
    DX12Program* FindProgram( ProgramHandle );

  private:
    DX12Program   mPrograms[ 100 ];
    ID3D12Device* mDevice{};
  };
} // namespace Tac::Render

