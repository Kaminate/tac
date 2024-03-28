#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{

  struct HLSLLinePreprocessorPadding : public HLSLLinePreprocessor
  {
    Optional< String > Preprocess( StringView, Errors& ) override;
    int mCounter = 0;
  };
} // namespace Tac::Render

