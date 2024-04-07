#pragma once

//#include "tac-std-lib/string/tac_string.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom
//#include "tac-win32/dx/dx12/tac_dx12_root_sig_bindings.h"

#include <d3d12.h> // ID3D12PipelineState
//#include <dxcapi.h> // (include after d3d12.h) IDxcBlob IDxcUtils, IDxcCompiler3, DxcCreateInstance

namespace Tac { struct Errors; }
namespace Tac::Render
{
  struct DX12Pipeline
  {
    PCom< ID3D12PipelineState > mPSO;
    PCom< ID3D12RootSignature > mRootSignature;
    //D3D12RootSigBindings        mRootSignatureBindings;
  };
} // namespace Tac::Render

