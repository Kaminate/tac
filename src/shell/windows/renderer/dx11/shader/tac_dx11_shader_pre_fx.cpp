#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/string/tac_string.h" // String
#include "src/common/preprocess/tac_preprocessor.h"
#include "src/common/error/tac_error_handling.h"

namespace Tac::Render
{
  Optional<String> PreprocessShaderLineDeprecations( const StringView line, Errors& errors )
  {
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-texture
    TAC_RAISE_ERROR_IF_RETURN(
      line.contains( "texture" ) && line.contains( "register" ),
      String() + "Using untyped texture in `" + line + "', "
      "Please used a typed texture like Texture2D, Texture2D<float4>, TextureCube, etc",
      {} );

    if( line.contains( "sampler_state" ) )
    {
      String replacement = line;
      replacement.replace( "sampler_state", "SamplerState" );
      TAC_RAISE_ERROR_RETURN( "Please replace `" + line + "` with `" + replacement + "`. "
                              "sampler_state is hella old",
                              {} );
    }


    if( line.contains( "sampler" ) && line.contains( "register" ) )
    {
      String replacement = line;
      replacement.replace( "sampler", "SamplerState");

      TAC_RAISE_ERROR_RETURN(
        String() + "Using `sampler` in `" + line + "', "
        "Please replace with `" + replacement + "` instead", // why? i dunno
        {} );
    }


    // https://www.gamedev.net/forums/topic/658935-samplerstate-in-code-or-hlsl/
    TAC_RAISE_ERROR_IF_RETURN(
      // The { and ; check if the sampler state opens a scope to define parameters
      line.contains( "SamplerState" ) && ( line.contains( '{' ) || !line.contains( ';' ) ),
      "Don't define the sampler properties in the shader, just create and bind sampler states "
      "manually. Occurance found in line `" + line + "`"
      , {} );

    return {};
  }
} // namespace Tac::Render



