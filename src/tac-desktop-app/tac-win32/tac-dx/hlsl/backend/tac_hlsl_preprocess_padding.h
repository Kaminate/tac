#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{

  struct HLSLLinePreprocessorPadding : public HLSLLinePreprocessor
  {
    Optional< String > Preprocess( Input, Errors& ) override;
    int mCounter {};
  };
} // namespace Tac::Render

