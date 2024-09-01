#include "tac-std-lib/meta/tac_meta_type.h"

#include "tac-std-lib/dataprocess/tac_json.h"

#pragma once

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  template< typename T >
  struct MetaIntegralType : public MetaType
  {
    MetaIntegralType( const char* name )                                                            { mName = name; }
    const char* GetName() const override                                                            { return mName; }
    int         GetSizeOf() const override                                                          { return sizeof( T ); }
    String      ToString( const void* v ) const override                                            { return Tac::ToString( (u64) As_T( v ) ); } // just u64 it here
    float       ToNumber( const void* v ) const override                                            { return ( float )As_T( v ); }
    void        Cast( CastParams castParams ) const override                                        { As_T( castParams.mDst ) = ( T )castParams.mSrcType->ToNumber( castParams.mSrc ); }
    void        JsonSerialize( Json* json, const void* v ) const override                          
    {
      const T t{ As_T( v ) };
      if( ( T )( JsonNumber )t == t )
        json->SetNumber( ( JsonNumber )t );
      else if( ( T )( int )t == t )
        json->SetString( Itoa( ( int )t ) );
      else
      {
        TAC_ASSERT_UNIMPLEMENTED;
      }
    }
    void        JsonDeserialize( const Json* json, void* v ) const override
    {   
      T& t { As_T( v ) };

      switch( json->mType )
      {
        case JsonType::Number: t = ( T )json->mNumber; break;
        case JsonType::String: t = ( T )Atoi( json->mString );  break;
        default: TAC_ASSERT_INVALID_CASE( json->mType ); break;
      }
    }

  private:

 
    const char* mName;
    const T&    As_T( const void* v ) const                                                         { return *(T*)v; }
    dynmc T&    As_T( dynmc void* v ) const                                                         { return *(T*)v; }
  };


  // -----------------------------------------------------------------------------------------------

} // namespace Tac

