#pragma once

#include "src/common/meta/tacmeta.h"
#include "src/common/containers/tacArray.h"

namespace Tac
{
  class MetaFnSig
  {
  public:
    template< typename Ret >
    MetaFnSig( Ret( *fn )( ) )
    {
      mRet = &GetMetaType< RetT >();
    }

    template< typename RetT, typename A0 >
    MetaFnSig( RetT( *)( A0 ) )
    {
      static Array sArgs{ &GetMetaType< A0 >() };
      mRet = &GetMetaType< RetT >();
      mArgs = sArgs.data();
      mArgCount = sArgs.size();
    }

    template< typename RetT, typename A0, typename A1 >
    MetaFnSig( RetT( *)( A0, A1 ) )
    {
      static Array sArgs{ &GetMetaType< A0 >(),
                          &GetMetaType< A1 >() };
      mRet = &GetMetaType< RetT >();
      mArgs = sArgs.data();
      mArgCount = sArgs.size();
    }

    const MetaType* RetType() const        { return mRet; }
    const MetaType* ArgType( int i ) const { return mArgs[ i ]; }
    int             ArgCount() const       { return mArgCount; }
  private:
    const MetaType*  mRet = nullptr;
    const MetaType** mArgs = nullptr;
    int              mArgCount = 0;
  };

  void MetaFnSigUnitTest();
}

