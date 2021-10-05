#include "src/common/meta/tacmetafn.h"
#include <iostream>

namespace Tac
{

  static int DoubleMyGuy( int i )
  {
    return i * 2;
  }

  static int AddMyGuys( int i, float f )
  {
    return i + ( int )f;
  }


  void MetaFnUnitTest0()
  {
    TAC_META_UNIT_TEST_TITLE;
    int doubleRet = 0;
    int doubleArg = 5;
    Variable doubleRetVar( doubleRet );
    Variable doubleArgVar( doubleArg );
    Apply( DoubleMyGuy, doubleRetVar, &doubleArgVar, 1 );
    std::cout << "Double( " << doubleArg << " ) = " << doubleRet << std::endl;
  }

  void MetaFnUnitTest1()
  {
    TAC_META_UNIT_TEST_TITLE;
    MetaFn doubleMeta( "double my guy", DoubleMyGuy );
    int doubleRet = 0;
    int doubleArg = 6;
    Variable doubleRetVar( doubleRet );
    Variable doubleArgVar( doubleArg );
    doubleMeta.Apply( doubleRetVar, &doubleArgVar, 1 );
    std::cout << doubleMeta.Name() << "( " << doubleArg << " ) = " << doubleRet << std::endl;
  }

  void MetaFnUnitTest2()
  {
    TAC_META_UNIT_TEST_TITLE;
    MetaFn addFn( "add my guys", AddMyGuys );
    int ret = 0;
    int arg0 = 10;
    float arg1 = 3.14f;
    Variable variableRet( ret );
    Array variables{ Variable( arg0 ), Variable( arg1 ) };
    addFn.Apply( ret, variables.data(), variables.size() );
    std::cout << addFn.Name() << "( " << arg0 << ", " << arg1 << " ) = " << ret << std::endl;
  }

  void MetaFnUnitTest()
  {
    TAC_META_UNIT_TEST_TITLE;
    MetaFnUnitTest0();
    MetaFnUnitTest1();
    MetaFnUnitTest2();
  }

  const char*     MetaFn::Name() const           { return mName; }
  const MetaType* MetaFn::RetType() const        { return mFnSig.RetType(); }
  const MetaType* MetaFn::ArgType( int i ) const { return mFnSig.ArgType( i ); }
  int             MetaFn::ArgCount() const       { return mFnSig.ArgCount(); }
  void            MetaFn::Apply( Variable ret, Variable* args, int argCount )
  {
    mApplyWrapper( mFn, ret, args, argCount );
  }
}

