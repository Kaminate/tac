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

    virtual auto Preprocess( Input, Errors& ) -> Optional< String > = 0;
  };

  struct HLSLFilePreprocessor
  {
    void Add( HLSLLinePreprocessor* );
    auto PreprocessFile( AssetPathStringView, Errors& ) -> String;
    auto PreprocessSource( StringView, Errors& ) -> String;

  private:
    auto PreprocessLine( HLSLLinePreprocessor::Input, Errors& ) -> Optional< String >;
    //auto PreprocessSourceChunk( StringView, Errors& ) -> String;

    Vector< HLSLLinePreprocessor* > mProcessors;
    AssetPathString                 mAssetPath;
  };

} // namespace Tac::Render

