#pragma once

#include "src/shell/windows/renderer/tac_dx.h" // PCom
#include "src/common/graphics/tac_renderer.h" // ShaderType
#include "src/common/assetmanagers/tac_asset.h" // AssetPathStringView
#include "src/common/system/tac_filesystem.h" // Filesystem::Path

// d3d12 must be included before dxcapi
#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance, 

namespace Tac::Render
{
  struct DX12DXCOutput
  {
    PCom<IDxcBlob>        mBlob;
    D3D12_SHADER_BYTECODE mByteCode;
  };

  struct DX12ShaderCompileFromStringInput
  {
    AssetPathStringView mShaderAssetPath;
    StringView          mPreprocessedShader;
    StringView          mEntryPoint;
    ShaderType          mType = ShaderType::Count;
    D3D_SHADER_MODEL    mShaderModel = (D3D_SHADER_MODEL)0;
    Filesystem::Path    mOutputDir;
  };

  DX12DXCOutput DX12CompileShaderDXC( const DX12ShaderCompileFromStringInput&, Errors& );

} // namespace Tac::Render

