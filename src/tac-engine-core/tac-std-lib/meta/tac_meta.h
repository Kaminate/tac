#pragma once

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/meta/tac_meta_type.h"
#include "tac-std-lib/meta/tac_meta_decl.h"
#include "tac-std-lib/meta/tac_meta_impl.h"

namespace Tac
{
  using ConstCharStar = const char*;
  TAC_META_DECL( ConstCharStar );
  TAC_META_DECL( float );
  TAC_META_DECL( double );
  TAC_META_DECL( bool );

  TAC_META_DECL( i8 );
  TAC_META_DECL( i16 );
  TAC_META_DECL( i32 );
  TAC_META_DECL( i64 );

  TAC_META_DECL( u8 );
  TAC_META_DECL( u16 );
  TAC_META_DECL( u32 );
  TAC_META_DECL( u64 );

  const MetaType&                        GetNullMetaType();

  // For GetMetaType() to work, files that define new meta types must be included before this one
  // ^ is this even true?
  template< typename T > const MetaType& GetMetaType()         { T t{}; return GetMetaType( t ); }
  template <> inline const MetaType&     GetMetaType< void >() { return GetNullMetaType(); }

#define TAC_GET_META_TYPE_DEFINED

  template < typename T > T              MetaCast( const void* v, const MetaType* m )
  {
    T t{};
    const MetaType::CastParams castParams
    {
      .mDst     { &t },
      .mSrc     { v },
      .mSrcType { m },
    };
    GetMetaType( t ).Cast( castParams );
    return t;
  }

  template< typename T >
  struct AutoLister
  {
    struct Iterator
    {
      Iterator( T* t ) : mT( t )            {}
      T*   operator*()                      { return mT; }
      void operator++()                     { mT = ( ( AutoLister* )mT )->mNext; }
      bool operator!=( const Iterator& it ) { return mT != it.mT; }

      T* mT;
    };

    struct Range
    {
      Iterator begin()                      { return Head(); }
      Iterator end()                        { return nullptr; }
    };

    static T*& Head()                       { static T* sHead; return sHead; }
    AutoLister()                            { mNext = Head(); Head() = ( T* )this; }

    T* mNext;
  };

  void RunMetaUnitTestSuite();
} // namespace Tac

