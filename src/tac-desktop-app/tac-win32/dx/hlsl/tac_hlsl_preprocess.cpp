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
    HLSLLinePreprocessorRegister procReg;
    HLSLLinePreprocessorIncludes procInc( assetPath );
    HLSLLinePreprocessorPadding procPad;
    HLSLLinePreprocessorComment procCmt;
    HLSLLinePreprocessorBitfield procBF;
    HLSLLinePreprocessorFx procFx;
    HLSLLinePreprocessorSemantic procAS;

    HLSLFilePreprocessor preprocessor;
    preprocessor.Add( &procReg );
    preprocessor.Add( &procInc );
    preprocessor.Add( &procPad );
    //preprocessor.Add( &procCmt ); // <-- no
    preprocessor.Add( &procBF );
    preprocessor.Add( &procFx );
    preprocessor.Add( &procAS );

    return preprocessor.PreprocessFile( assetPath, errors );
  }
} // namespace Tac::Render

