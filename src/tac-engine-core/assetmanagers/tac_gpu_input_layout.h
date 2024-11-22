#pragma once

#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::Render
{
  struct GPUInputLayout
  {
    GPUInputLayout() = default;
    GPUInputLayout( const VertexDeclarations& );

    static constexpr int N{ 16 };
    using uint = u32;

    uint     mGraphicsTypes[ N ] {};
    uint     mElementCounts[ N ] {};
    uint     mByteOffsets[ N ]   {};
    uint     mStride             {};
  };

} // namespace Tac
