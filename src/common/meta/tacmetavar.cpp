#include "src/common/meta/tacmetavar.h"
#include <iostream>

namespace Tac
{
  void MetaPrintVariables()
  {
    for( MetaVar* var : MetaVar::Range() )
    {
      std::cout
        << var->mName
        << "="
        << var->mType.ToString( var->mAddr )
        << std::endl;
    }
  }

  void MetaVarUnitTest()
  {
    TAC_META_UNIT_TEST_TITLE;
    int i = 42;
    float f = 3.14f;
    TAC_META_REGISTER_VAR( i );
    TAC_META_REGISTER_VAR( f );
    MetaPrintVariables();
  }
}

