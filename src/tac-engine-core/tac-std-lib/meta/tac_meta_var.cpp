#include "tac_meta_var.h" // self-inc

#include "tac-std-lib/dataprocess/tac_log.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  void MetaPrintVariables()
  {
    for( MetaVar* var : MetaVar::Range() )
      LogApi::LogMessagePrintLine(
        String()
        + var->mName
        + " = "
        + var->mType.ToString( var->mAddr ) );
  }

  void MetaVarUnitTest()
  {
    int i { 42 };
    float f { 3.14f };
    TAC_META_REGISTER_VAR( i );
    TAC_META_REGISTER_VAR( f );
    MetaPrintVariables();
  }
} // namespace Tac

