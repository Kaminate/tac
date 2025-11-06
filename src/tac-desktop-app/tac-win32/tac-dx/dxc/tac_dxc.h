// This file serves as a wrapper around dxc
#pragma once

#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-engine-core/asset/tac_asset.h" // AssetPathStringView
#include "tac-std-lib/filesystem/tac_filesystem.h" // FileSys::Path

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance
#include <d3d12shader.h> // Shader reflection (ID3D12ShaderReflection/D3D12_SHADER_INPUT_BIND_DESC)

namespace Tac::Render
{
  // Does not contain the input source file(s), only what the compiler needs
  struct DXCCompileParams
  {
    String           mFileName            {}; // ie: "foo.hlsl"

    // ie: "C:/Users/Nate/AppData/Roaming/Sleeping Studio/DX12 Hello Triangle"
    // Used to have shader source and pdb in a gpu debugger
    FileSys::Path    mOutputDir           {};

    StringView       mPreprocessedShader  {};
    D3D_SHADER_MODEL mShaderModel         {};
  };

  struct DXCReflInfo
  {
    void AddBinding( D3D12_SHADER_INPUT_BIND_DESC );
    bool HasBinding( StringView );

    struct Input
    {
      String mName     {};
      int    mIndex    {};
      int    mRegister {};
    };

    using Inputs            = Vector< Input >;
    using Blobs             = Vector< PCom < IDxcBlob > >;
    using BindDescs         = Vector< D3D12_SHADER_INPUT_BIND_DESC >;
    using ShaderReflections = Vector< PCom< ID3D12ShaderReflection > >;

    Blobs             mReflBlobs    {};
    ShaderReflections mRefls        {};
    BindDescs         mReflBindings {}; // Combined bindings from all shader stanges
    Inputs            mInputs       {};
  };

  struct DXCCompileOutput
  {
    auto GetVSBytecode() const -> D3D12_SHADER_BYTECODE;
    auto GetPSBytecode() const -> D3D12_SHADER_BYTECODE;
    auto GetCSBytecode() const -> D3D12_SHADER_BYTECODE;

    PCom< IDxcBlob > mVSBlob;
    PCom< IDxcBlob > mPSBlob;
    PCom< IDxcBlob > mCSBlob;
    DXCReflInfo      mReflInfo;
  };

  auto DXCCompile( const DXCCompileParams&, Errors& ) -> DXCCompileOutput;

} // namespace Tac::Render

