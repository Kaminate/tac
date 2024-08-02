#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
#include "tac-dx/dx12/program/tac_dx12_program_bindings.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h> // D3D12_SHADER_BYTECODE
#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac::Render
{
  struct DX12Program
  {
    String                mFileStem;
    FileSys::Time         mFileTime;

    PCom< IDxcBlob >      mVSBlob;
    D3D12_SHADER_BYTECODE mVSBytecode;

    PCom< IDxcBlob >      mPSBlob;
    D3D12_SHADER_BYTECODE mPSBytecode;

    PCom< IDxcBlob >      mCSBlob;
    D3D12_SHADER_BYTECODE mCSBytecode;

    D3D12ProgramBindings  mProgramBindings;
    ProgramParams         mProgramParams;

    struct Input
    {
      String mName; // this is a semantic name
      int    mIndex;
      int    mRegister;
    };
    Vector< Input >       mInputs;
  };
} // namespace Tac::Render

