#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{
  struct HLSLLinePreprocessorComment : HLSLLinePreprocessor
  {
    auto Preprocess( Input, Errors& ) -> Optional< String > override;
  };

} // namespace Tac::Render



