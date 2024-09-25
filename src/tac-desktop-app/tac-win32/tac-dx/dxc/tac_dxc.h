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
  struct DXCCompileParams
  {
    StringView       mFileName            {}; // ie: "foo.hlsl"
    StringView       mPreprocessedShader  {};
    D3D_SHADER_MODEL mShaderModel         {};
    FileSys::Path    mOutputDir           {};
  };

  struct DXCReflInfo
  {
    void AddBinding( D3D12_SHADER_INPUT_BIND_DESC );
    bool HasBinding( StringView );

    struct Input
    {
      String mName;
      int    mIndex;
      int    mRegister;
    };

    using Inputs            = Vector< Input >;
    using Blobs             = Vector< PCom < IDxcBlob > >;
    using BindDescs         = Vector< D3D12_SHADER_INPUT_BIND_DESC >;
    using ShaderReflections = Vector< PCom< ID3D12ShaderReflection > >;

    //                Storing IDxcBlob / ID3D12ShaderReflection because I assume that the string
    //                pointers in the D3D12_SHADER_INPUT_BIND_DESC go inside one of these
    Blobs             mReflBlobs;

    ShaderReflections mRefls;

    //                Combined bindings from all shader stanges
    BindDescs         mReflBindings;

    Inputs            mInputs;
  };

  struct DXCCompileOutput
  {
    D3D12_SHADER_BYTECODE GetVSBytecode() const;
    D3D12_SHADER_BYTECODE GetPSBytecode() const;
    D3D12_SHADER_BYTECODE GetCSBytecode() const;

    PCom< IDxcBlob > mVSBlob;
    PCom< IDxcBlob > mPSBlob;
    PCom< IDxcBlob > mCSBlob;
    DXCReflInfo      mReflInfo;
  };

  //enum DXC;

  DXCCompileOutput DXCCompile( const DXCCompileParams&, Errors& );

} // namespace Tac::Render

