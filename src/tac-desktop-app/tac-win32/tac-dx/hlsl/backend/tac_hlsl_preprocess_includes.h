#pragma once

#include "tac_hlsl_preprocess_backend.h"

namespace Tac::Render
{
  struct HLSLLinePreprocessorIncludes : public HLSLLinePreprocessor
  {
    HLSLLinePreprocessorIncludes( AssetPathStringView );

    Optional< String > Preprocess( Input, Errors& ) override;

  private:
    String IncludeFile( AssetPathStringView , Errors& );

    Vector< String > mIncluded;
    AssetPathString  mAssetPath;
  };
}


