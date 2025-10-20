#include "tac_hlsl_preprocess_fx.h" // self-inc

#include "tac-std-lib/string/tac_string.h" // String
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Render
{
  Optional< String > HLSLLinePreprocessorFx::Preprocess( Input input, Errors& errors )
  {
    const StringView line{ input.mLine };

    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-texture
    TAC_RAISE_ERROR_IF_RETURN( 
      line.contains( "texture" ) && line.contains( "register" ),
      String() + "Using untyped texture in `" + line + "', "
      "Please used a typed texture like Texture2D, Texture2D<float4>, TextureCube, etc" );

    if( line.contains( "sampler_state" ) )
    {
      String replacement { line };
      replacement.replace( "sampler_state", "SamplerState" );
      TAC_RAISE_ERROR_RETURN( "Please replace `" + line + "` with `" + replacement + "`. "
                              "sampler_state is hella old" );
    }


    if( line.contains( "sampler" ) && line.contains( "register" ) )
    {
      String replacement { line };
      replacement.replace( "sampler", "SamplerState");
      TAC_RAISE_ERROR_RETURN(
        String() + "Using `sampler` in `" + line + "', "
        "Please replace with `" + replacement + "` instead" // why? i dunno
        );
    }


    // https://www.gamedev.net/forums/topic/658935-samplerstate-in-code-or-hlsl/
    TAC_RAISE_ERROR_IF_RETURN(
      // The { and ; check if the sampler state opens a scope to define parameters
      line.contains( "SamplerState" ) && ( line.contains( '{' ) || !line.contains( ';' ) ),
      "Don't define the sampler properties in the shader, just create and bind sampler states "
      "manually. Occurance found in line `" + line + "`"
       );

    return {};
  }
} // namespace Tac::Render



