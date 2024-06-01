#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{

  struct HLSLLinePreprocessorSemantic : HLSLLinePreprocessor
  {
    Optional< String > Preprocess( Input, Errors& ) override;
  };

} // namespace Tac::Render



