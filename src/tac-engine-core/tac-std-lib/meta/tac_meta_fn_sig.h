#pragma once

#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac
{
  class MetaFnSig
  {
  public:
    template< typename Ret >
    MetaFnSig( Ret( *fn )( ) )
    {
      mRet = &GetMetaType< Ret >();
    }

    template< typename Ret, typename A0 >
    MetaFnSig( Ret( *)( A0 ) )
    {
      static Array sArgs{ &GetMetaType< A0 >() };
      mRet = &GetMetaType< Ret >();
      mArgs = sArgs.data();
      mArgCount = sArgs.size();
    }

    template< typename Ret, typename A0, typename A1 >
    MetaFnSig( Ret( *)( A0, A1 ) )
    {
      static Array sArgs
      {
        &GetMetaType< A0 >(),
        &GetMetaType< A1 >()
      };
      mRet = &GetMetaType< Ret >();
      mArgs = sArgs.data();
      mArgCount = sArgs.size();
    }

    const MetaType* RetType() const        { return mRet; }
    const MetaType* ArgType( int i ) const { return mArgs[ i ]; }
    int             ArgCount() const       { return mArgCount; }
  private:
    const MetaType*  mRet { nullptr };
    const MetaType** mArgs { nullptr };
    int              mArgCount { 0 };
  };

  void MetaFnSigUnitTest();
}

