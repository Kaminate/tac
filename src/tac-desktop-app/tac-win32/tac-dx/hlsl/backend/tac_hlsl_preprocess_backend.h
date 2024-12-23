#pragma once

#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/memory/tac_memory.h"

namespace Tac::Render
{
  struct HLSLLinePreprocessor
  {
    struct Input
    {
      StringView          mLine;
      AssetPathStringView mFile;
      int                 mLineNumber;
    };

    virtual Optional< String > Preprocess( Input, Errors& ) = 0;
  };

  struct HLSLFilePreprocessor
  {
    void   Add( HLSLLinePreprocessor* );
    String PreprocessFile( AssetPathStringView, Errors& );
    String PreprocessSource( StringView, Errors& );

  private:
    Optional< String > PreprocessLine( HLSLLinePreprocessor::Input, Errors& );

    Vector< HLSLLinePreprocessor* > mProcessors;
    AssetPathString                 mAssetPath;
  };

} // namespace Tac::Render

