#pragma once

#include "tac-win32/dx/dx12/shaderprogram/tac_dx12_shader_program.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  struct DX12ShaderProgramMgr
  {
    void Init( ID3D12Device*, Errors& );
    void CreateShaderProgram( ProgramHandle, ShaderProgramParams, Errors& );
    void DestroyProgram(ProgramHandle);
    DX12ShaderProgram* FindProgram( ProgramHandle );

  private:
    DX12ShaderProgram mShaderPrograms[ 100 ];
    ID3D12Device*     mDevice{};
  };
} // namespace Tac::Render

