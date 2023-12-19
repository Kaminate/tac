#include "src/shell/windows/renderer/dx11/shader/tac_dx11_shader_preprocess.h" // self-inc

#include "src/common/assetmanagers/tac_asset.h"
#include "src/common/containers/tac_array.h"
#include "src/common/error/tac_error_handling.h"
#include "src/common/dataprocess/tac_text_parser.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_renderer_backend.h"
#include "src/common/math/tac_math.h"
#include "src/common/memory/tac_frame_memory.h"

namespace Tac::Render
{
  bool IsSingleLineCommented( const StringView& line )
  {
    ParseData shaderParseData( line.data(), line.size() );
    shaderParseData.EatWhitespace();
    return shaderParseData.EatStringExpected( "//" );
  }

  static String PreprocessShaderLine( const StringView& line, Errors& errors )
  {
    if( IsSingleLineCommented( line ) )
      return line;

    const String bitfield = PreprocessShaderBitfield( line );
    const String fxFramework = PreprocessShaderFXFramework( line );
    const String inc = TAC_CALL_RET( line, PreprocessShaderIncludes( line, errors ) );
    const String pad = PreprocessShaderPadding( line );
    const String reg = PreprocessShaderRegister( line );
    const String semanticName = PreprocessShaderSemanticName( line );
    const String processedLines[] = { bitfield, fxFramework, inc, pad, reg, semanticName };

    for( const String& processedLine : processedLines )
      if( processedLine != line )
        return PreprocessShaderSource( processedLine, errors );

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
      result += '\n';
    }

    --recurseCount;
    return result;
  }

} // namespace Tac::Render



