#pragma once

#include "tac_meta_type.h"
#include "tac_meta_impl.h"
#include "tac_meta_integral.h"
#include "tac-std-lib/containers/tac_vector.h"

#include <type_traits>

namespace Tac
{

  template< typename T >
  struct MetaEnum : public MetaIntegralType< std::underlying_type_t< T > >
  {
    Vector< String > mValStrs;
    Vector< T >      mValNums;

    dynmc T& AsEnum( dynmc void* v ) const { return ( dynmc T& )MetaIntegralType < std::underlying_type_t< T > >::As_T( v ); }
    const T& AsEnum( const void* v ) const { return ( const T& )MetaIntegralType < std::underlying_type_t< T > >::As_T( v ); }

    int    GetSizeOf() const override  { return (int)sizeof( T ); }
    String ToString( const void* v ) const override
    {
      const T t{ AsEnum( v ) };
      const int n{ Count() };
      for( int i{}; i < n; ++i )
        if( mValNums[ i ] == t )
          return mValStrs[ i ];

      TAC_ASSERT_INVALID_CODE_PATH;
      return n ? mValStrs[ 0 ] : "";
    }

    void Cast( MetaType::CastParams castParams ) const override                                       
    {
      TAC_ASSERT_INVALID_CODE_PATH;
    }

    void JsonSerialize( dynmc Json* json, const void* v ) const override
    {
      json->SetString( ToString( v ) );
    }

    void JsonDeserialize( const Json* json, dynmc void* v ) const override
    {
      TAC_ASSERT( json->mType == JsonType::String );
      dynmc T& t{ AsEnum( v ) };
      const int n{ Count() };
      for( int i{}; i < n; ++i )
      {
        if( mValStrs[ i ] == json->mString )
        {
          t = mValNums[ i ];
          return;
        }
      }

      TAC_ASSERT_INVALID_CODE_PATH;
      t = n ? mValNums[ 0 ] : ( T )0;
    }

    int Count() const
    {
      return mValNums.size(); // same as mValStrs.size
    }
    void AddValue( T num, StringView str )
    {
      mValNums.push_back( num );
      mValStrs.push_back( str );
    }
  };

} // namespace Tac


#define TAC_META_REGISTER_ENUM_BEGIN( T )  \
struct Meta##T : public Tac::MetaEnum< T > \
{                                          \
  using value_type = T;                    \
  Meta##T()                                \
  {                                        \
    SetName( #T )
#define TAC_META_REGISTER_ENUM_VALUE( val ) AddValue( value_type::val, #val )
#define TAC_META_REGISTER_ENUM_END( T )    \
  }                                        \
};                                         \
TAC_META_IMPL( T )
//static Meta##T sMeta##T( #T );             \
//TAC_META_IMPL_INSTANCE( T, sMeta##T )
