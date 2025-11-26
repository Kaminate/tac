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

  auto GetNullMetaType() -> const MetaType&;

  // For GetMetaType() to work, files that define new meta types must be included before this one
  // ^ is this even true?
  template< typename T >
  auto GetMetaType() -> const MetaType&         { T t{}; return GetMetaType( t ); }

  template <> inline
  auto GetMetaType< void >() -> const MetaType& { return GetNullMetaType(); }

#define TAC_GET_META_TYPE_DEFINED

  template < typename T >
  auto MetaCast( const void* v, const MetaType* m ) -> T
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
      auto operator*() -> T*                { return mT; }
      void operator++()                     { mT = ( ( AutoLister* )mT )->mNext; }
      bool operator!=( const Iterator& it ) { return mT != it.mT; }

      T* mT;
    };

    struct Range
    {
      auto begin() -> Iterator              { return Head(); }
      auto end() -> Iterator                { return nullptr; }
    };

    static auto Head() -> T*&               { static T* sHead; return sHead; }
    AutoLister()                            { mNext = Head(); Head() = ( T* )this; }

    T* mNext;
  };

  void RunMetaUnitTestSuite();
} // namespace Tac

