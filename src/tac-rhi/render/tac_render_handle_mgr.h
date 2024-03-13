#pragma once

#include "tac-rhi/render/tac_render_handle.h"

namespace Tac::Render
{
  Handle RenderHandleAlloc( HandleType );
  void   RenderHandleFree( HandleType, Handle );
} // namespace Tac::Render

