#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/containers/tac_array.h"
#include "src/common/core/tac_error_handling.h"
#include "src/common/dataprocess/tac_text_parser.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_renderer_backend.h"
#include "src/common/math/tac_math.h"
#include "src/common/memory/tac_frame_memory.h"

namespace Tac::Render
{
  static bool IsSingleLineCommented( const StringView& line )
  {
    ParseData shaderParseData( line.data(), line.size() );
    shaderParseData.EatWhitespace();
    return shaderParseData.EatStringExpected( "//" );
  }

  static String PreprocessShaderLine( const StringView& line, Errors& errors )
  {
    if( IsSingleLineCommented( line ) )
      return line;

    const String preprocessedLines[] =
    {
      PreprocessShaderBitfield( line ),
      PreprocessShaderFXFramework( line ),
      PreprocessShaderIncludes( line, errors ),
      PreprocessShaderPadding( line ),
      PreprocessShaderRegister( line ),
      PreprocessShaderSemanticName( line ),
    };
    TAC_HANDLE_ERROR_RETURN( line );

    for( const String& s : preprocessedLines )
    {
      if( s != line )
      {
        return PreprocessShaderSource( s, errors );
      }
    }

    return line;
  }

  String PreprocessShaderSource( const StringView& shaderSourceCode, Errors& errors )
  {
    static int recurseCount;
    if( !recurseCount )
      ResetShaderRegisters();

    ++recurseCount;

    String result;
    ParseData shaderParseData( shaderSourceCode.data(), shaderSourceCode.size() );
    while( shaderParseData.GetRemainingByteCount() )
    {
      const StringView line = shaderParseData.EatRestOfLine();
      result += PreprocessShaderLine( line , errors );
      result + '\n';
    }

    --recurseCount;
    return result;
  }

} // namespace Tac::Render



