#pragma once

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (must be included after d3d12.h) IDxc..., Dxc...

#include "src/common/tac_core.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/shell/windows/tac_win32_com_ptr.h"

namespace Tac::Render
{
  struct DX12ProgramCompiler
  {
    struct Result
    {
      D3D12_SHADER_BYTECODE GetBytecode( ShaderType );
      PCom<IDxcBlob> mBlobs[ (int) ShaderType::Count ];
    };

    DX12ProgramCompiler( ID3D12Device*, Errors& );
    Result Compile( AssetPathStringView, Errors& );

  };

} // namespace Tac::Render

