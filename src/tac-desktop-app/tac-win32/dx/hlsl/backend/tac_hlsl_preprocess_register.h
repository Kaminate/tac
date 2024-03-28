#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{

  struct HLSLLinePreprocessorRegister : HLSLLinePreprocessor
  {
    Optional< String > Preprocess( StringView, Errors& ) override;
    int                Add( char, int );

    int                mLetterCounts[ 128 ]{};
  };

} // namespace Tac::Render



