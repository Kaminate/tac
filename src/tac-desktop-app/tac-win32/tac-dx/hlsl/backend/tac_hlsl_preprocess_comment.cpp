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

  auto HLSLLinePreprocessorComment::Preprocess( Input input, Errors& ) -> Optional< String >
  {
    StringView line{ input.mLine };
    const bool isComment { IsSingleLineCommented( line ) };
    return isComment ? Optional< String >( line ) : Optional< String >{};
  }
} // namespace Tac::Render



