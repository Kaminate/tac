// This file serves as a wrapper around dxc
#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-std-lib/filesystem/tac_asset.h" // AssetPathStringView
#include "tac-std-lib/filesystem/tac_filesystem.h" // Filesystem::Path

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac::Render
{
  struct DXCCompileParams
  {
    StringView          mFileName; // ie: "foo.hlsl"
    StringView          mPreprocessedShader;
    D3D_SHADER_MODEL    mShaderModel = ( D3D_SHADER_MODEL )0;
    Filesystem::Path    mOutputDir;
  };

  struct DXCCompileOutput
  {
    PCom< IDxcBlob > mVSBlob;
    PCom< IDxcBlob > mPSBlob;
  };

  DXCCompileOutput DXCCompile( const DXCCompileParams&, Errors& );

} // namespace Tac::Render

