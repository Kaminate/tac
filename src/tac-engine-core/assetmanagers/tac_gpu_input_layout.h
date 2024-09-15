#pragma once

#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  struct GPUInputLayout
  {
    GPUInputLayout() = default;
    GPUInputLayout( const VertexDeclarations& );

    u8      mElementCounts[ 16 ] {};
    u8      mGraphicsTypes[ 16 ] {};
    u8      mByteOfffsets[ 16 ]  {};
    u32     mStride              {};
  };
} // namespace Tac
