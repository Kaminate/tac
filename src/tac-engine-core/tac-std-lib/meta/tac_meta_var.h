#pragma once

#include "tac-std-lib/meta/tac_meta.h"

namespace Tac
{


  struct MetaVar : public AutoLister< MetaVar >
  {
    template< typename T > MetaVar( const char* name, T* t ) :
      mName( name ),
      mAddr( t ),
      mType( GetMetaType( *t ) )
    {}

    const char*     mName;
    void*           mAddr;
    const MetaType& mType;

    // line number, file...
  };


#define TAC_META_REGISTER_VAR( v ) \
  static MetaVar gMetaVar##v(#v, &v);

  void MetaPrintVariables();
  void MetaVarUnitTest();


}

