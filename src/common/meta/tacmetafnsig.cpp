#include "src/common/meta/tacmetafnsig.h"
#include <iostream>

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
    std::cout << "Ret: " << fnSig.RetType()->GetName() << std::endl;
    for( int i = 0; i < fnSig.ArgCount(); ++i )
    {
      std::cout << "Arg " << i << ": " << fnSig.ArgType( i )->GetName() << std::endl;
    }
  }

  void MetaFnSigUnitTest()
  {
    MetaFnSig my_fn_sig( Foo );
    std::cout << "Foo()" << std::endl;
    PrintMetaFnSig( my_fn_sig );

    MetaFnSig my_fn_sig_2( Qux );
    std::cout << "Qux()" << std::endl;
    PrintMetaFnSig( my_fn_sig_2 );
  }

}

