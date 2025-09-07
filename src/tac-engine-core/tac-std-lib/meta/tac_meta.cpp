#include "tac_meta.h" // self-inc

#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/meta/tac_meta_integral.h"

namespace Tac
{
  // How should we handle failure?
  static auto TryGetJsonMetapointer( const Json* json ) -> const void*
  {
    switch( json->mType )
    {
      case JsonType::Number: return &json->mNumber;
      case JsonType::Bool:   return &json->mBoolean;
      case JsonType::String: return &json->mString;
      default:               return nullptr;
    }
  }

  // How should we handle failure?
  static auto TryGetJsonMetatype( const Json* json ) -> const MetaType*
  {
    switch( json->mType )
    {
      case JsonType::Number: return &GetMetaType< decltype( json->mNumber ) >();
      case JsonType::Bool: return &GetMetaType< decltype( json->mBoolean ) >();
      default: return nullptr;
    }
  }

  // -----------------------------------------------------------------------------------------------

  struct MetaBool : public MetaType
  {
    const char* GetName() const override { return "bool"; }
    int         GetSizeOf() const override { return sizeof( bool ); }
    String      ToString( const void* v ) const override { return AsBool( v ) ? "true" : "false"; }
    float       ToNumber( const void* v ) const override { return AsBool( v ) ? 1.0f : 0.0f; }
    void        Cast( CastParams castParams ) const override { AsBool( castParams.mDst ) = ( bool )castParams.mSrcType->ToNumber( castParams.mSrc ); }
    void        JsonSerialize( Json* json, const void* v ) const override { json->SetBool( AsBool( v ) ); }
    void        JsonDeserialize( const Json* json, void* v ) const override
    {
      if( json->mType == JsonType::Bool )
      {
        AsBool( v ) = json->mBoolean;
        return;
      }

      const CastParams castParams
      {
        .mDst     { v },
        .mSrc     { TryGetJsonMetapointer( json ) },
        .mSrcType { TryGetJsonMetatype( json ) },
      };
      Cast( castParams );
    }
    bool        Equals( const void* a, const void* b ) const override { return AsBool( a ) == AsBool( b ); }
    void        Copy( CopyParams cp ) const override { AsBool( cp.mDst ) = AsBool( cp.mSrc ); }

  private:
    const char* mName{};
    const bool& AsBool( const void* v ) const { return *( bool* )v; }
    dynmc bool& AsBool( dynmc void* v ) const { return *( bool* )v; }
  };

  struct MetaConstCharStar : public MetaType
  {
    const char* GetName() const override { return "char*"; }
    int         GetSizeOf() const override { return sizeof( char* ); }
    String      ToString( const void* v ) const override { return AsStr( v ); }
    float       ToNumber( const void* v ) const override { return ( float )Atoi( AsStr( v ) ); }
    void        Cast( CastParams ) const override { TAC_ASSERT_INVALID_CODE_PATH; }
    void        JsonSerialize( Json* json, const void* v ) const override { json->SetString( AsStr( v ) ); }
    void        JsonDeserialize( const Json*, void* ) const override { TAC_ASSERT_INVALID_CODE_PATH; }

  private:
    const char* AsStr( const void* v ) const { return ( const char* )v; }
  };

  struct MetaFloat : public MetaType
  {
    const char* GetName() const override { return "float"; }
    int         GetSizeOf() const override { return sizeof( float ); }
    String      ToString( const void* v ) const override { return Tac::ToString( AsFloatRef( v ) ); }
    float       ToNumber( const void* v ) const override { return AsFloatRef( v ); }
    void        Cast( CastParams castParams ) const override
    {
      *( float* )castParams.mDst = ( float )castParams.mSrcType->ToNumber( castParams.mSrc );
    }
    void        JsonSerialize( Json* json, const void* v ) const override
    {
      json->SetNumber( MetaCast< JsonNumber >( v, this ) );
    }
    void        JsonDeserialize( const Json* json, void* v ) const override
    {
      const CastParams castParams
      {
        .mDst     { v },
        .mSrc     { TryGetJsonMetapointer( json ) },
        .mSrcType { TryGetJsonMetatype( json ) },
      };
      Cast( castParams );
    }
    bool        Equals( const void* a, const void* b ) const override { return AsFloatRef( a ) == AsFloatRef( b ); }
    void        Copy( CopyParams cp ) const override { AsFloatRef( cp.mDst ) = AsFloatRef( cp.mSrc ); }

  private:
    const float& AsFloatRef( const void* v ) const { return *( float* )v; }
    dynmc float& AsFloatRef( dynmc void* v ) const { return *( float* )v; }
  };

  struct MetaDouble : public MetaType
  {
    const char* GetName() const override { return "double"; }
    int         GetSizeOf() const override { return sizeof( double ); }
    String      ToString( const void* v ) const override { return Tac::ToString( AsDoubleRef( v ) ); }
    float       ToNumber( const void* v ) const override { return ( float )AsDoubleRef( v ); }
    void        Cast( CastParams castParams ) const override
    {
      *( double* )castParams.mDst = ( double )castParams.mSrcType->ToNumber( castParams.mSrc );
    }
    void        JsonSerialize( Json* json, const void* v ) const override
    {
      json->SetNumber( MetaCast< JsonNumber >( v, this ) );
    }
    void        JsonDeserialize( const Json* json, void* v ) const override
    {
      const CastParams castParams
      {
        .mDst     { v },
        .mSrc     { TryGetJsonMetapointer( json ) },
        .mSrcType { TryGetJsonMetatype( json ) },
      };
      Cast( castParams );
    }
    bool        Equals( const void* a, const void* b ) const override { return AsDoubleRef( a ) == AsDoubleRef( b ); }
    void        Copy( CopyParams cp ) const override { AsDoubleRef( cp.mDst ) = AsDoubleRef( cp.mSrc ); }

  private:
    const double& AsDoubleRef( const void* v ) const { return *( double* )v; }
    dynmc double& AsDoubleRef( dynmc void* v ) const { return *( double* )v; }
  };

  struct MetaNull : public MetaType
  {
    const char* GetName() const override                             { return "null"; }
    int         GetSizeOf() const override                           { return 0; }
    String      ToString( const void* ) const override               { return "null"; }
    float       ToNumber( const void* ) const override               { return 0; }
    void        Cast( CastParams ) const override                    { TAC_ASSERT_INVALID_CODE_PATH; }
    void        JsonSerialize( Json*, const void* ) const override   { TAC_ASSERT_INVALID_CODE_PATH; }
    void        JsonDeserialize( const Json*, void* ) const override { TAC_ASSERT_INVALID_CODE_PATH; }
  };

  struct Metai8  : public MetaIntegralType< i8 >  { Metai8()  : MetaIntegralType( "i8" )  {} };
  struct Metai16 : public MetaIntegralType< i16 > { Metai16() : MetaIntegralType( "i16" ) {} };
  struct Metai32 : public MetaIntegralType< i32 > { Metai32() : MetaIntegralType( "i32" ) {} };
  struct Metai64 : public MetaIntegralType< i64 > { Metai64() : MetaIntegralType( "i64" ) {} };

  struct Metau8  : public MetaIntegralType< u8 >  { Metau8()  : MetaIntegralType( "u8" )  {} };
  struct Metau16 : public MetaIntegralType< u16 > { Metau16() : MetaIntegralType( "u16" ) {} };
  struct Metau32 : public MetaIntegralType< u32 > { Metau32() : MetaIntegralType( "u32" ) {} };
  struct Metau64 : public MetaIntegralType< u64 > { Metau64() : MetaIntegralType( "u64" ) {} };

  using ConstCharStar = const char*;

  TAC_META_IMPL_INSTANCE2( float, MetaFloat );
  TAC_META_IMPL_INSTANCE2( double, MetaDouble );
  TAC_META_IMPL_INSTANCE2( bool, MetaBool );

  TAC_META_IMPL_INSTANCE2( i8, Metai8 );
  TAC_META_IMPL_INSTANCE2( i16, Metai16  );
  TAC_META_IMPL_INSTANCE2( i32, Metai32  );
  TAC_META_IMPL_INSTANCE2( i64, Metai64  );

  TAC_META_IMPL_INSTANCE2( u8, Metau8 );
  TAC_META_IMPL_INSTANCE2( u16, Metau16 );
  TAC_META_IMPL_INSTANCE2( u32, Metau32 );
  TAC_META_IMPL_INSTANCE2( u64, Metau64 );

  TAC_META_IMPL_INSTANCE2( ConstCharStar, MetaConstCharStar );
}

const Tac::MetaType& Tac::GetNullMetaType() { static const MetaNull sMetaNull; return sMetaNull; }

