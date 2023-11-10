#pragma once

#include "src/shell/windows/renderer/tac_renderer_directx11.h"

namespace Tac::Render
{
  void PostprocessShaderSource( const StringView&, ConstantBuffers* );
} // namespace Tac::Render

