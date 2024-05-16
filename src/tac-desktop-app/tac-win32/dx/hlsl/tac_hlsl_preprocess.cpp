#include "tac_hlsl_preprocess.h" // self-inc

#include "tac-win32/dx/hlsl/tac_hlsl_preprocess.h"
#include "tac-win32/dx/hlsl/backend/tac_hlsl_preprocess_bitfield.h"
#include "tac-win32/dx/hlsl/backend/tac_hlsl_preprocess_comment.h"
#include "tac-win32/dx/hlsl/backend/tac_hlsl_preprocess_fx.h"
#include "tac-win32/dx/hlsl/backend/tac_hlsl_preprocess_includes.h"
#include "tac-win32/dx/hlsl/backend/tac_hlsl_preprocess_padding.h"
#include "tac-win32/dx/hlsl/backend/tac_hlsl_preprocess_register.h"
#include "tac-win32/dx/hlsl/backend/tac_hlsl_preprocess_semantic.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Render
{
  String HLSLPreprocess( AssetPathStringView assetPath, Errors& errors )
  {
    HLSLLinePreprocessorRegister processorRegister;
    HLSLLinePreprocessorIncludes processorIncludes( assetPath );
    HLSLLinePreprocessorPadding processorPadding;
    HLSLLinePreprocessorComment processorComment;
    HLSLLinePreprocessorBitfield processorBitfield;
    HLSLLinePreprocessorFx processorFx;
    HLSLLinePreprocessorSemantic processorSemantic;

    HLSLFilePreprocessor preprocessor;
    preprocessor.Add( &processorComment ); // first
    preprocessor.Add( &processorRegister );
    preprocessor.Add( &processorIncludes );
    preprocessor.Add( &processorPadding );
    preprocessor.Add( &processorBitfield );
    preprocessor.Add( &processorFx );
    preprocessor.Add( &processorSemantic );

    return preprocessor.PreprocessFile( assetPath, errors );
  }
} // namespace Tac::Render

