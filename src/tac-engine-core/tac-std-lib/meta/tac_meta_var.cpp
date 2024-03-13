#include "tac_meta_var.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  void MetaPrintVariables()
  {
    for( MetaVar* var : MetaVar::Range() )
      OS::OSDebugPrintLine( String() + var->mName + " = " + var->mType.ToString( var->mAddr ) );
  }

  void MetaVarUnitTest()
  {
    int i = 42;
    float f = 3.14f;
    TAC_META_REGISTER_VAR( i );
    TAC_META_REGISTER_VAR( f );
    MetaPrintVariables();
  }
} // namespace Tac

