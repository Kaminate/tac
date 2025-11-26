#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-dx/dx12/program/tac_dx12_program_bind_desc.h"
#include "tac-dx/dxc/tac_dxc.h" // DXCReflInfo
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac::Render
{
  struct DX12Program
  {
    struct HotReloadInput
    {
      UTF8Path mFilePath {};
      FileTime mFileTime {};
    };

    struct HotReloadInputs : public Vector< HotReloadInput >
    {
      HotReloadInputs() = default;
      HotReloadInputs( const ProgramParams&, Errors& );
    };

    struct Input
    {
      String mName     {}; // this is a semantic name
      int    mIndex    {};
      int    mRegister {};
    };

    struct Inputs : public Vector< Input >
    {
      Inputs() = default;
      Inputs( const DXCReflInfo::Inputs& );
    };

    PCom< IDxcBlob >         mVSBlob           {};
    PCom< IDxcBlob >         mPSBlob           {};
    PCom< IDxcBlob >         mCSBlob           {};
    D3D12_SHADER_BYTECODE    mVSBytecode       {};
    D3D12_SHADER_BYTECODE    mPSBytecode       {};
    D3D12_SHADER_BYTECODE    mCSBytecode       {};
    D3D12ProgramBindDescs    mProgramBindDescs {};
    ProgramParams            mProgramParams    {};
    Inputs                   mInputs           {};
    Vector< HotReloadInput > mHotReloadInputs  {};
  };
} // namespace Tac::Render

