#pragma once

#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  // must mirror the struct in inputlayout.hlsl
  struct GPUInputLayout
  {
    GPUInputLayout() = default;
    GPUInputLayout( const VertexDeclarations& );

    static constexpr int N{ 16 };

    u32     mGraphicsTypes[ N ] {};
    u32     mElementCounts[ N ] {};
    u32     mByteOffsets[ N ]   {};
    u32     mStride             {};
  };

} // namespace Tac
