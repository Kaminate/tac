#include "tac_hlsl_preprocess.h" // self-inc

#include "tac-dx/hlsl/tac_hlsl_preprocess.h"
#include "tac-dx/hlsl/backend/tac_hlsl_preprocess_bitfield.h"
#include "tac-dx/hlsl/backend/tac_hlsl_preprocess_comment.h"
#include "tac-dx/hlsl/backend/tac_hlsl_preprocess_fx.h"
#include "tac-dx/hlsl/backend/tac_hlsl_preprocess_includes.h"
#include "tac-dx/hlsl/backend/tac_hlsl_preprocess_padding.h"
#include "tac-dx/hlsl/backend/tac_hlsl_preprocess_register.h"
#include "tac-dx/hlsl/backend/tac_hlsl_preprocess_semantic.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Render
{
  auto HLSLPreprocessor::Process( Vector< AssetPathString > assetPaths, Errors& errors ) -> String
  {
    HLSLLinePreprocessorRegister processorRegister;
    HLSLLinePreprocessorIncludes processorIncludes;
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

    String result;
    for( const AssetPathString& assetPath : assetPaths )
    {
      TAC_CALL_RET( result += preprocessor.PreprocessFile( assetPath, errors ) );
      result += "\n";
    }

    return result;
  }
} // namespace Tac::Render

