#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{
  struct HLSLLinePreprocessorIncludes : public HLSLLinePreprocessor
  {
    auto Preprocess( Input, Errors& ) -> Optional< String > override ;

  private:
    auto IncludeFile( AssetPathStringView , Errors& ) -> String;
    bool IsIncluded( AssetPathString ) const;

    Vector< AssetPathString > mIncluded;
  };
}


