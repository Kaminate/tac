#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/meta/tac_meta_fn_sig.h"

namespace Tac
{
  // not MetaVariable?
  struct Variable
  {
    Variable() : mType( &GetMetaType< int >() ), mAddr( nullptr ) {}

    template< typename T >
    Variable( const T&t ) : mType( &GetMetaType( t ) ), mAddr( ( void* )&t ) {}

    const MetaType* mType;
    void*           mAddr;
  };

  template < typename T >
  auto MetaCast( Variable v ) -> T
  {
    T t{};
    const MetaType::CastParams castParams
    {
      .mDst     { &t },
      .mSrc     { v.mAddr },
      .mSrcType { v.mType },
    };
    GetMetaType( t ).Cast( castParams );
    return t;
  }

  using FnPtr = void( *)( );

  template< typename Ret >
  void Apply( Ret fnRes, Variable ret )
  {
    const MetaType::CastParams castParams
    {
      .mDst     { ret.mAddr },
      .mSrc     { &fnRes },
      .mSrcType { ret.mType },
    };
    ret.mType->Cast( castParams );
  }

  template< typename Ret >
  void Apply( Ret( *fn )( ), Variable ret, Variable* args, int argCount )
  {
    Apply( fn(), ret );
  }

  template< typename Ret, typename A0 >
  void Apply( Ret( *fn )( A0 ), Variable ret, Variable* args, int argCount )
  {
    TAC_UNUSED_PARAMETER(argCount); // ???
    Apply( fn( MetaCast< A0 >( args[ 0 ] ) ), ret );
  }

  template< typename Ret, typename A0, typename A1 >
  void Apply( Ret( *fn )( A0, A1 ), Variable ret, Variable* args, int argCount )
  {
    TAC_UNUSED_PARAMETER( argCount );
    Apply( fn( MetaCast< A0 >( args[ 0 ] ),
               MetaCast< A1 >( args[ 1 ] ) ), ret );
  }

  template< typename Fn >
  void ApplyWrapper( FnPtr fnPtr, Variable ret, Variable* args, int argCount )
  {
    TAC_UNUSED_PARAMETER(argCount);
    Apply( ( Fn )fnPtr, ret, args, argCount );
  }

  struct MetaFn : public AutoLister< MetaFn >
  {
  public:
    template< typename Fn >
    MetaFn( const char* name, Fn fn )
      : mName( name )
      , mFnSig( fn )
      , mFn( ( FnPtr )fn )
      , mApplyWrapper( ApplyWrapper< Fn > )
    {

    }

    auto Name() const -> const char*;
    auto RetType() const -> const MetaType*;
    auto ArgType( int ) const -> const MetaType*;
    auto ArgCount() const -> int;
    void Apply( Variable ret, Variable* args, int argCount );

  private:
    using ApplyFn = void( * )( FnPtr, Variable, Variable*, int );
    const char* mName;
    MetaFnSig   mFnSig;
    FnPtr       mFn;
    ApplyFn     mApplyWrapper;
  };

  void MetaPrintFunctions();
  void MetaFnUnitTest();

} // namespace Tac

