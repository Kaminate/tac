// This file serves as a wrapper around dxc
#pragma once

#include "src/shell/windows/tac_win32_com_ptr.h" // PCom
#include "src/common/graphics/tac_renderer.h" // ShaderType
#include "src/common/assetmanagers/tac_asset.h" // AssetPathStringView
#include "src/common/system/tac_filesystem.h" // Filesystem::Path

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac::Render::DXC
{
  struct Input
  {
    AssetPathStringView mShaderAssetPath;
    StringView          mPreprocessedShader;
    StringView          mEntryPoint;
    ShaderType          mType = ShaderType::Count;
    D3D_SHADER_MODEL    mShaderModel = (D3D_SHADER_MODEL)0;
    Filesystem::Path    mOutputDir;
  };
  
  PCom<IDxcBlob> Compile( const Input&, Errors& );

} // namespace Tac::Render

