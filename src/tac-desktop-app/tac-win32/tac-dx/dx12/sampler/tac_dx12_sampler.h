#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_allocation.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom

#include <d3d12.h>

namespace Tac { struct Errors; }
namespace Tac::Render
{
  struct DX12Sampler
  {
    DX12Descriptor mDescriptor;
    String         mName;
  };
} // namespace Tac::Render

