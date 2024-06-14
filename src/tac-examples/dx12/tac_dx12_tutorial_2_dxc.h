// use tac_dxc.h instead
#if 0
// This file serves as a wrapper around dxc
#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-rhi/render3/tac_render_api.h" // ShaderType
#include "tac-engine-core/asset/tac_asset.h" // AssetPathStringView
#include "tac-std-lib/filesystem/tac_filesystem.h" // FileSys::Path

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac::Render::DXC
{
  struct ExampleInput
  {
    AssetPathStringView mShaderAssetPath;
    StringView          mPreprocessedShader;
    StringView          mEntryPoint;
    ShaderType          mType = ShaderType::Count;
    D3D_SHADER_MODEL    mShaderModel = ( D3D_SHADER_MODEL )0;
    FileSys::Path    mOutputDir;
  };

  PCom< IDxcBlob > ExampleCompile( const ExampleInput&, Errors& );

} // namespace Tac::Render

#endif
