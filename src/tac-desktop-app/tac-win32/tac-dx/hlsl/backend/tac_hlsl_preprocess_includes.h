#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{
  struct HLSLLinePreprocessorIncludes : public HLSLLinePreprocessor
  {
    Optional< String > Preprocess( Input, Errors& ) override;

  private:
    String IncludeFile( AssetPathStringView , Errors& );
    bool IsIncluded( AssetPathString ) const;

    Vector< AssetPathString > mIncluded;
  };
}


