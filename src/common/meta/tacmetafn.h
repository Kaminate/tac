#pragma once

#include "src/common/meta/tacmeta.h"
#include "src/common/meta/tacmetafnsig.h"

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

  template < typename T > T MetaCast( Variable v )
  {
    T t;
    GetMetaType( t ).Cast( &t, v.mAddr, v.mType );
    return t;
  }

  using FnPtr = void( *)( );

  template< typename Ret >
  void Apply( Ret fnRes, Variable ret )
  {
    ret.mType->Cast( ret.mAddr, &fnRes, ret.mType );
  }

  template< typename Ret >
  void Apply( Ret( *fn )( ), Variable ret, Variable* args, int argCount )
  {
    Apply( fn(), ret );
  }

  template< typename Ret, typename A0 >
  void Apply( Ret( *fn )( A0 ), Variable ret, Variable* args, int argCount )
  {
    Apply( fn( MetaCast< A0 >( args[ 0 ] ) ), ret );
  }

  template< typename Ret, typename A0, typename A1 >
  void Apply( Ret( *fn )( A0, A1 ), Variable ret, Variable* args, int argCount )
  {
    Apply( fn( MetaCast< A0 >( args[ 0 ] ),
               MetaCast< A1 >( args[ 1 ] ) ), ret );
  }

  template< typename Fn >
  void ApplyWrapper( FnPtr fnPtr, Variable ret, Variable* args, int argCount )
  {
    Apply( ( Fn )fnPtr, ret, args, argCount );
  }

  struct MetaFn : public AutoLister< MetaFn >
  {
  public:
    template< typename Fn >
    MetaFn( const char* name, Fn fn )
      : mName( name )
      , mFnSig( fn )
      , mApplyWrapper( ApplyWrapper< Fn > )
      , mFn( ( FnPtr )fn )
    {

    }

    const char*     Name() const;
    const MetaType* RetType() const;
    const MetaType* ArgType( int ) const;
    int             ArgCount() const;
    void            Apply( Variable ret, Variable* args, int argCount );

  private:
    const char* mName;
    MetaFnSig   mFnSig;
    FnPtr       mFn;
    void( *mApplyWrapper )( FnPtr, Variable, Variable*, int );
  };

  void MetaFnUnitTest();

} // namespace Tac

