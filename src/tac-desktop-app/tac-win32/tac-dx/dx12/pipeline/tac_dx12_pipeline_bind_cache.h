// The purpose of this file is to keep track of what resources are
// bound to 
//
// 

#pragma once

#include "tac-dx/dx12/descriptor/tac_dx12_descriptor_heap_gpu_mgr.h" // DX12DescriptorRegion
#include "tac-std-lib/containers/tac_vector.h"

namespace Tac::Render
{
  struct PipelineBindCache
  {
    struct Table
    {
      DX12DescriptorRegion mRegion;
    };

    Vector< Table > mRootTables;
  };
}

