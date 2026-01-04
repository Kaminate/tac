#pragma once

#include "tac-std-lib/meta/tac_meta_type.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac
{

  template< typename T, int N >
  struct MetaArrayT : public MetaType
  {
    String mName;

    MetaArrayT()
    {
      const MetaType& metaT{ GetMetaType< T >() };
      mName = String() + "Array<" + metaT.GetName() + "," + Tac::ToString( N ) + ">";
    }
    auto GetName() const -> const char* override               { return mName.c_str(); }
    auto GetSizeOf() const -> int override                     { return sizeof( Array< T, N > ); }
    auto ToString( const void* ) const -> String override      { TAC_ASSERT_INVALID_CODE_PATH; return ""; }
    auto ToNumber( const void* ) const -> float override       { TAC_ASSERT_INVALID_CODE_PATH; return 0; }
    void Cast( CastParams ) const override                     { TAC_ASSERT_UNIMPLEMENTED; }
    void JsonSerialize( dynmc Json* json, const void* v ) const override
    {
      const MetaType& metaT{ GetMetaType< T >() };
      const Array< T, N >& ts{ *( Array< T, N >* )v };
      json->Clear();
      for( const T& t : ts )
        metaT.JsonSerialize( json->AddChild(), &t );
    }
    void JsonDeserialize( const Json* json, dynmc void* v ) const override
    {
      const MetaType& metaT{ GetMetaType< T >() };
      dynmc Array< T, N >& ts{ *( Array< T, N >* )v };
      const int n{json->mArrayElements.size()};
      TAC_ASSERT( n == N );
      for( int i{}; i < n; ++i )
        metaT.JsonDeserialize( json->mArrayElements[ i ], &ts[ i ] );
    }
    void Copy( CopyParams params ) const override
    {
      Array< T, N >* dst{ ( Array< T, N >* )params.mDst };
      Array< T, N >* src{ ( Array< T, N >* )params.mSrc };
      *dst = *src;
      //for( int i{}; i < N; ++i )
      //  dst[ i ] = src[ i ];
      //const MetaType& metaT{ GetMetaType< T >() };
      //for( int i{}; i < N; ++i )
      //  metaT.Copy(
      //    CopyParams
      //    {
      //      .mDst{ (char*)params.mDst + i * sizeof(N) },
      //      .mSrc{ (char*)params.mSrc + i * sizeof(N) }
      //    } );
    }

    static auto Instance() -> MetaArrayT<T, N>&
    {
      static MetaArrayT<T, N> sInstance;
      return sInstance;
    }
  };

  template< typename T, int N >
  auto GetMetaType( Array< T, N >& ) -> const MetaType&
  {
    return MetaArrayT< T, N >::Instance();
  }

} // namespace Tac

