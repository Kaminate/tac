#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{
  struct HLSLLinePreprocessorBitfield : public HLSLLinePreprocessor
  {
    Optional< String > Preprocess( StringView, Errors& ) override;

    int  mRunningBitCount{};
    bool mProcessing{};
  };
}



