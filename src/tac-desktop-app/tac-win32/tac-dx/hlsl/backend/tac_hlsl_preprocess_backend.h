#pragma once

#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_span.h"
#include "tac-std-lib/memory/tac_memory.h"

namespace Tac::Render
{
  //struct HLSLFilePreprocessor;
  struct HLSLLinePreprocessor
  {
    virtual Optional<String> Preprocess( StringView, Errors& ) = 0;
    //HLSLFilePreprocessor* mParent{};
  };

  struct HLSLFilePreprocessor
  {
    void   Add( HLSLLinePreprocessor* );
    String PreprocessFile( AssetPathStringView, Errors& );
    String PreprocessSource( StringView, Errors& );

  private:
    Optional< String > PreprocessLine( StringView, Errors& );

    Vector< HLSLLinePreprocessor* > mProcessors;
    AssetPathString                 mAssetPath;
  };

} // namespace Tac::Render

