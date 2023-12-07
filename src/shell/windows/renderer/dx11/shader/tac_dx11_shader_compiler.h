#pragma once

#include "src/common/graphics/tac_renderer.h"

#include <d3dcommon.h> // WKPDID_D3DDebugObjectName, ID3DBlob

namespace Tac::Render
{

  ID3DBlob* CompileShaderFromString( const ShaderNameStringView& shaderSource,
                                     const StringView& shaderStrOrig,
                                     const StringView& shaderStrFull,
                                     const StringView& entryPoint,
                                     const StringView& shaderModel,
                                     Errors& errors );
} // namespace Tac::Render

