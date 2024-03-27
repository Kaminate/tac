// This file serves as a wrapper around dxc
#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-rhi/renderer/tac_renderer.h" // ShaderType
#include "tac-std-lib/filesystem/tac_asset.h" // AssetPathStringView
#include "tac-std-lib/filesystem/tac_filesystem.h" // Filesystem::Path

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac::Render
{
  struct DXCInput
  {
    AssetPathStringView mShaderAssetPath;
    StringView          mPreprocessedShader;
    StringView          mEntryPoint;
    ShaderType          mType = ShaderType::Count;
    D3D_SHADER_MODEL    mShaderModel = ( D3D_SHADER_MODEL )0;
    Filesystem::Path    mOutputDir;
  };

  PCom< IDxcBlob > DXCCompile( const DXCInput&, Errors& );

} // namespace Tac::Render

