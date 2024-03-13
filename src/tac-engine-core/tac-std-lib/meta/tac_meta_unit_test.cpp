#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta_fn.h"
#include "tac-std-lib/meta/tac_meta_fn_sig.h"
#include "tac-std-lib/meta/tac_meta_var.h"
#include "tac-std-lib/os/tac_os.h"

namespace Tac
{
  static void MetaUnitTestBigHeader( const char* title )
  {
    OS::OSDebugPrintLine("");
    OS::OSDebugPrintLine("//");
    OS::OSDebugPrintLine("// " + String() + title);
    OS::OSDebugPrintLine("//");
    OS::OSDebugPrintLine("");
  }

  static void MetaUnitTestTitle( const char* title )
  {
    const String dashes = "-----------";
    OS::OSDebugPrintLine("");
    OS::OSDebugPrintLine(dashes + " Meta Unit Test - " + title + dashes );
  }

  static void MetaUnitTest( const char* title, void( *fn )( ) )
  {
    MetaUnitTestTitle( title );
    fn();
  }

  void RunMetaUnitTestSuite()
  {
    MetaUnitTestBigHeader( "Meta Test Suite Begin" );
    MetaUnitTest( "Var",           MetaVarUnitTest );
    MetaUnitTest( "Composite Var", MetaCompositeUnitTest );
    MetaUnitTest( "FnSig",         MetaFnSigUnitTest );
    MetaUnitTest( "Fn",            MetaFnUnitTest );
    MetaUnitTestBigHeader( "Meta Test Suite End" );
  }

} // namespace Tac

