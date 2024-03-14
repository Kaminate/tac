#pragma once

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (must be included after d3d12.h) IDxc..., Dxc...

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac { struct Errors; struct AssetPathStringView; }
namespace Tac::Render
{
  struct DX12ProgramCompiler
  {
    struct Result
    {
      D3D12_SHADER_BYTECODE GetBytecode( ShaderType );
      PCom<IDxcBlob> mBlobs[ (int) ShaderType::Count ];
    };

    struct Params
    {
      Filesystem::Path mOutputDir;
      ID3D12Device* mDevice;
    };

    DX12ProgramCompiler( Params, Errors& );
    Result Compile( const AssetPathStringView&, Errors& );

  };

} // namespace Tac::Render

