#include "tac_meta_fn_sig.h" // self-inc

#include "tac-std-lib/dataprocess/tac_log.h"
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
    for( int i {}; i < fnSig.ArgCount(); ++i )
    {
      args += sep + "Arg " + ToString( i ) + ": " + fnSig.ArgType( i )->GetName();
      sep = ", ";
    }

    LogApi::LogMessagePrintLine( String() + "Ret: " + fnSig.RetType()->GetName() + "( " + args + ")" );
  }

  void MetaFnSigUnitTest()
  {
    LogApi::LogMessagePrintLine( "The signature of Foo is: " );
    PrintMetaFnSig( MetaFnSig( Foo ) );

    LogApi::LogMessagePrintLine( "The signature of Qux is: " );
    PrintMetaFnSig( MetaFnSig( Qux ) );
  }

} // namespace Tac

