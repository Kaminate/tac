#pragma once

#include "src/common/tac_core.h"
#include "src/common/assetmanagers/tac_asset.h"
//#include "src/common/string/tac_string.h"
#include "src/common/containers/tac_optional.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/containers/tac_span.h"
#include "src/common/memory/tac_memory.h"

namespace Tac::Render
{

  using ShaderPreprocessFn = Optional<String> (*)( const StringView, Errors& );

  struct IShaderLinePreprocessor
  {
    virtual Optional<String> Preprocess( const StringView, Errors& ) = 0;
  };

  struct ShaderPreprocessor
  {
    void Add( IShaderLinePreprocessor* );
    void Add( ShaderPreprocessFn );

    //Span< IShaderLinePreprocessor* > GetProcessors();

    Optional<String> PreprocessLine( const StringView, Errors& );

    AssetPathString mAssetPath;

  private:
    Vector< IShaderLinePreprocessor* > mProcessors;
    Vector< ShaderPreprocessFn > mFns;
  };

  //struct DX11ShaderPreprocessor
  //{
  //  //linepreproecss;
  //  //filepreprocessor;
  //};

  String DXPreprocessShaderSource( StringView, ShaderPreprocessor*, Errors& );

} // namespace Tac::Render

