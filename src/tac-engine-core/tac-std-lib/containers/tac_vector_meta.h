#pragma once

#include "tac-std-lib/meta/tac_meta_type.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/dataprocess/tac_json.h"

namespace Tac
{

  template< typename T >
  struct MetaVectorT : public MetaType
  {
    String mName;

    MetaVectorT()
    {
      const MetaType& metaT{ GetMetaType< T >() };
      mName += "Vector<";
      mName += metaT.GetName();
      mName += "Vector>";
    }
    const char* GetName() const override               { return mName.c_str(); }
    int         GetSizeOf() const override             { return sizeof( Vector< T > ); }
    String      ToString( const void* ) const override { TAC_ASSERT_INVALID_CODE_PATH; return ""; }
    float       ToNumber( const void* ) const override { TAC_ASSERT_INVALID_CODE_PATH; return 0; }
    void        Cast( CastParams ) const override      { TAC_ASSERT_UNIMPLEMENTED; }
    void        JsonSerialize( dynmc Json* json, const void* v) const override
    {
      const MetaType& metaT{ GetMetaType< T >() };
      const Vector< T >& ts{ *( Vector< T >* )v };
      json->Clear();
      for( const T& t : ts )
        metaT.JsonSerialize( json->AddChild(), &t );
    }
    void        JsonDeserialize( const Json* json, dynmc void* v) const override
    {
      const MetaType& metaT{ GetMetaType< T >() };
      dynmc Vector< T >& ts{ *( Vector< T >* )v };
      ts.clear();
      const int n{json->mArrayElements.size()};
      ts.resize( n );
      for( int i{}; i < n; ++i )
        metaT.JsonDeserialize( json->mArrayElements[ i ], &ts[ i ] );
    }

    static MetaVectorT<T>& Instance()
    {
      static MetaVectorT<T> sInstance;
      return sInstance;
    }
  };

  template< typename T >
  const MetaType& GetMetaType( Vector< T >& )
  {
    return MetaVectorT< T >::Instance();
  }

} // namespace Tac

