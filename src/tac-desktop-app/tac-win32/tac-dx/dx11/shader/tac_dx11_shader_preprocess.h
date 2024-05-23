#pragma once

#include "tac-std-lib/containers/tac_optional.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-win32/dx/dx_shader_preprocess/tac_dx_shader_preprocess.h"

namespace Tac { struct StringView; struct AssetPathStringView; struct Errors; }

namespace Tac::Render
{

  // The shader register map is used in conjunction with the TAC_AUTO_REGISTER shader macro
  // to assign unique registers to each shader resource.
  struct ShaderPreprocessorRegister : public IShaderLinePreprocessor
  {
    Optional<String> Preprocess( const StringView, Errors& ) override;
  private:
    int              Add( char, int );
    int              mLetterCounts[ 128 ]{};
  };

  struct ShaderPreprocessorIncludes : public IShaderLinePreprocessor
  {
    ShaderPreprocessorIncludes( AssetPathString );
    Optional<String> Preprocess( const StringView, Errors& ) override;
  private:
    Vector< String > mIncluded;
    AssetPathString  mAssetPath;
  };

  struct ShaderPreprocessorPadding : public IShaderLinePreprocessor
  {
    Optional<String> Preprocess( const StringView, Errors& ) override;
  private:
    int mCounter = 0;
  };


  struct ShaderPreprocessorFunction : public IShaderLinePreprocessor
  {
    ShaderPreprocessorFunction( ShaderPreprocessFn );
    Optional<String> Preprocess( const StringView , Errors&  ) override;
  private:
    ShaderPreprocessFn mFn { nullptr };
  };

  String           DX11PreprocessShader( const AssetPathStringView& assetPath, Errors& );

  Optional<String> PreprocessShaderLineComment( const StringView , Errors&);
  Optional<String> PreprocessShaderLineBitfield( const StringView , Errors&);
  Optional<String> PreprocessShaderLineDeprecations( const StringView , Errors&);
  Optional<String> PreprocessShaderLineSemanticName( const StringView, Errors& );

  bool IsSingleLineCommented( const StringView& );

} // namespace Tac::Render

