#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{

  struct HLSLLinePreprocessorRegister : HLSLLinePreprocessor
  {
    auto Preprocess( Input, Errors& ) -> Optional< String > override;
    auto Add( char, int ) -> int;
    int mLetterCounts[ 128 ]{};
  };

} // namespace Tac::Render



