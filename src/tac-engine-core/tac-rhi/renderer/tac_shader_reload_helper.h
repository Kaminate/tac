#pragma once

#include "tac-rhi/renderer/tac_renderer.h"

namespace Tac::Render
{
  using ShaderReloadFunction = void( ShaderHandle, const ShaderNameStringView&, Errors& );
  void ShaderReloadHelperAdd( ShaderHandle, const ShaderNameStringView&, Errors& );
  void ShaderReloadHelperRemove( ShaderHandle );
  void ShaderReloadHelperUpdate( float dt, ShaderReloadFunction*, Errors& );

} // namespace Tac::Render
