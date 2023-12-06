#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/string/tac_string.h" // String
#include "src/common/core/tac_preprocessor.h"

namespace Tac::Render
{
  String PreprocessShaderFXFramework( const StringView& line )
  {
    // At a base level, D3D11 only supports creating sampler states on the application side using D3D API calls.
    // Any HLSL code you've seen for defining sampler states was using the effects framework,
    // which is a higher-level library that sits on top of core D3D.
    // The effects framework actually reflects the sampler data from the compiled code,
    // and uses that reflected data to create and bind sampler states behind the scenes.
    // I really wouldn't recommend using the effects framework (it's old and no longer updated),
    // so you should just create and bind sampler states manually.
    // If necessary, you can always write your own layer for reflecting data from your shader and
    // using that to control binding of samplers and textures.
    //
    // ( quote from MJP 2014-07-19 on gamedev.net )

    const char* fxFrameworkStrs[] = { "SamplerState" }; // Are there more?
    for( const char* s : fxFrameworkStrs )
    {
      TAC_ASSERT_MSG( !line.contains( s ),
                      String( s ) + " "
                      "is a deprecated directx fx framework feature and wont be supported "
                      "in the newer DXIL compiler. Create and bind your resources manually." );
    }

    return line;
  }
} // namespace Tac::Render



