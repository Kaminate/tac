#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{

  struct HLSLLinePreprocessorFx : public HLSLLinePreprocessor
  {
    auto Preprocess( Input, Errors& ) -> Optional< String > override;
  };

}



