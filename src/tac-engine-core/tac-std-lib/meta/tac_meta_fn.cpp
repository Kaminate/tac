#include "tac_meta_fn.h" // self-inc

#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"

namespace Tac
{

  static int DoubleMyGuy( int i )
  {
    return i * 2;
  }
  static MetaFn doubleMeta( "DoubleMyGuy", DoubleMyGuy );

  static int AddMyGuys( int i, float f )
  {
    return i + ( int )f;
  }
  static MetaFn addFn( "AddMyGuys", AddMyGuys );

  static void MetaFnUnitTest0()
  {
    int doubleRet {};
    int doubleArg { 5 };
    Variable doubleRetVar( doubleRet );
    Variable doubleArgVar( doubleArg );
    Apply( DoubleMyGuy, doubleRetVar, &doubleArgVar, 1 );
    OS::OSDebugPrintLine(
      String()
      + "Double( " + ToString( doubleArg ) + " ) = " + ToString( doubleRet ) );
  }

  static void MetaFnUnitTest1()
  {
    int doubleRet {};
    int doubleArg { 6 };
    Variable doubleRetVar( doubleRet );
    Variable doubleArgVar( doubleArg );
    doubleMeta.Apply( doubleRetVar, &doubleArgVar, 1 );
    const char* fnname = doubleMeta.Name();
    OS::OSDebugPrintLine(
      String() + fnname + "( " + ToString( doubleArg) + " ) = " + ToString( doubleRet ) );
  }

  static void MetaFnUnitTest2()
  {
    int ret {};
    int arg0 { 10 };
    float arg1 { 3.14f };
    Variable variableRet( ret );
    Array variables{ Variable( arg0 ), Variable( arg1 ) };
    addFn.Apply( ret, variables.data(), variables.size() );
    const char* fnname = addFn.Name();
    OS::OSDebugPrintLine(
      String()
      + fnname + "( "
      + ToString( arg0 ) + ", "
      + ToString( arg1 ) + " ) = "
      + ToString( ret ) );
  }

  auto MetaFn::Name() const -> const char*                { return mName; }
  auto MetaFn::RetType() const -> const MetaType*         { return mFnSig.RetType(); }
  auto MetaFn::ArgType( int i ) const -> const MetaType*  { return mFnSig.ArgType( i ); }
  auto MetaFn::ArgCount() const -> int                    { return mFnSig.ArgCount(); }
  void MetaFn::Apply( Variable ret, Variable* args, int argCount )
  {
    mApplyWrapper( mFn, ret, args, argCount );
  }
} // namespace Tac

void Tac::MetaFnUnitTest()
{
  MetaFnUnitTest0();
  MetaFnUnitTest1();
  MetaFnUnitTest2();
}

void Tac::MetaPrintFunctions()
{
  for( MetaFn* fn : MetaFn::Range() )
  {
    String joinedArgs;
    const char* sep = "";
    for( int i {}; i < fn->ArgCount(); ++i )
    {
      joinedArgs += sep;
      joinedArgs += fn->ArgType( i )->GetName();
      sep = ", ";
    }

    String line;
    line += fn->RetType()->GetName();
    line += ' ';
    line += fn->Name();
    line += '(';
    line += joinedArgs;
    line += ')';
    line += '\n';

    OS::OSDebugPrintLine( line );
  }
}

