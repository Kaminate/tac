#pragma once

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (must be included after d3d12.h) IDxc..., Dxc...

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-win32/tac_win32_com_ptr.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac { struct Errors; struct AssetPathStringView; }
namespace Tac::Render
{
  struct DX12ExampleProgramCompiler
  {
    struct Result
    {
      PCom< IDxcBlob >      mVSBlob;
      PCom< IDxcBlob >      mPSBlob;
      D3D12_SHADER_BYTECODE mVSBytecode;
      D3D12_SHADER_BYTECODE mPSBytecode;
    };

    struct Params
    {
      Filesystem::Path mOutputDir;
      ID3D12Device*    mDevice;
    };

    DX12ExampleProgramCompiler( Params, Errors& );
    Result Compile( const AssetPathStringView&, Errors& );

  };

} // namespace Tac::Render

