#pragma once

#include "src/common/graphics/tac_renderer_backend_2.h"

namespace Tac::Render
{
  // you know, do we even inherit form renderer?
  // this is our chance to rebuild the renderer
  struct DX12RendererVer2 : public Renderer2
  {
  };
}
