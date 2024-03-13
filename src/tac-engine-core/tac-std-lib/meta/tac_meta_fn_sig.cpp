#include "tac_meta_fn_sig.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{
  static int Foo( float f )
  {
    return ( int )( f + 3.63f );
  }

  static int Qux( float f, int i )
  {
    return ( int )( f + 3.63f + i );
  }

  void PrintMetaFnSig( MetaFnSig fnSig )
  {
    String sep;
    String args;
    for( int i = 0; i < fnSig.ArgCount(); ++i )
    {
      args += sep + "Arg " + ToString( i ) + ": " + fnSig.ArgType( i )->GetName();
      sep = ", ";
    }

    OS::OSDebugPrintLine( String() + "Ret: " + fnSig.RetType()->GetName() + "( " + args + ")" );
  }

  void MetaFnSigUnitTest()
  {
    OS::OSDebugPrintLine( "The signature of Foo is: " );
    PrintMetaFnSig( MetaFnSig( Foo ) );

    OS::OSDebugPrintLine( "The signature of Qux is: " );
    PrintMetaFnSig( MetaFnSig( Qux ) );
  }

} // namespace Tac

