#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{
  struct HLSLLinePreprocessorBitfield : public HLSLLinePreprocessor
  {
    auto Preprocess( Input, Errors& ) -> Optional< String > override;

    int  mRunningBitCount{};
    bool mProcessing{};
  };
}



