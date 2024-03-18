#pragma once

#include "tac-win32/dx/dx11/tac_renderer_dx11.h"

namespace Tac::Render
{
  DX11Program DX11LoadProgram( const ShaderNameStringView& , Errors& );
} // namespace Tac::Render

