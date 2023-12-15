#pragma once

#include "src/common/tac_common.h"

namespace Tac::Render
{
  String PreprocessShaderSource( const StringView&, Errors& );

  String PreprocessShaderBitfield( const StringView& );
  String PreprocessShaderFXFramework( const StringView& );
  String PreprocessShaderIncludes( const StringView&, Errors& );
  String PreprocessShaderPadding( const StringView& );
  String PreprocessShaderRegister( const StringView& );
  String PreprocessShaderSemanticName( const StringView& );
  bool IsSingleLineCommented( const StringView& );

  void ResetShaderRegisters();
} // namespace Tac::Render



