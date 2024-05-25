#pragma once

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-win32/tac_win32_com_ptr.h" // PCom

#include <d3dcommon.h> // WKPDID_D3DDebugObjectName, ID3DBlob

namespace Tac::Render
{

  PCom<ID3DBlob> CompileShaderFromString( const ShaderNameStringView& shaderSource,
                                          const StringView& shaderStrOrig,
                                          const StringView& shaderStrFull,
                                          const StringView& entryPoint,
                                          const StringView& shaderModel,
                                          Errors& errors );
} // namespace Tac::Render
