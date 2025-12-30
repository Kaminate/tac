#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{

  struct HLSLLinePreprocessorPadding : public HLSLLinePreprocessor
  {
    auto Preprocess( Input, Errors& ) -> Optional< String > override;
    int mCounter {};
  };
} // namespace Tac::Render

