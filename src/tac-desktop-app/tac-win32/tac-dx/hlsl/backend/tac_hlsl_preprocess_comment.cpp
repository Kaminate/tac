#include "tac_hlsl_preprocess_comment.h" // self-inc

#include "tac-std-lib/dataprocess/tac_text_parser.h"

namespace Tac::Render
{
  static bool IsSingleLineCommented( StringView line )
  {
    ParseData shaderParseData( line.data(), line.size() );
    shaderParseData.EatWhitespace();
    return shaderParseData.EatStringExpected( "//" );
  }

  Optional< String > HLSLLinePreprocessorComment::Preprocess( StringView line, Errors& )
  {
    const bool isComment { IsSingleLineCommented( line ) };
    return isComment ? Optional< String >( line ) : Optional< String >{};
  }
} // namespace Tac::Render



