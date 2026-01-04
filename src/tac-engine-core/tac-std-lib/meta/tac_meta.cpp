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
    auto GetName() const -> const char* override { return "bool"; }
    auto GetSizeOf() const -> int override { return sizeof( bool ); }
    auto ToString( const void* v ) const -> String override { return AsBool( v ) ? "true" : "false"; }
    auto ToNumber( const void* v ) const -> float override { return AsBool( v ) ? 1.0f : 0.0f; }
    void Cast( CastParams castParams ) const override { AsBool( castParams.mDst ) = ( bool )castParams.mSrcType->ToNumber( castParams.mSrc ); }
    void JsonSerialize( Json* json, const void* v ) const override { json->SetBool( AsBool( v ) ); }
    void JsonDeserialize( const Json* json, void* v ) const override
    {
      if( json->mType == JsonType::Bool )
        AsBool( v ) = json->mBoolean;
      else
        Cast(
          CastParams
          {
            .mDst     { v },
            .mSrc     { TryGetJsonMetapointer( json ) },
            .mSrcType { TryGetJsonMetatype( json ) },
          } );
    }
    bool Equals( const void* a, const void* b ) const override { return AsBool( a ) == AsBool( b ); }
    void Copy( CopyParams cp ) const override { AsBool( cp.mDst ) = AsBool( cp.mSrc ); }

  private:
    auto AsBool( const void* v ) const -> const bool& { return *( bool* )v; }
    auto AsBool( dynmc void* v ) const -> dynmc bool& { return *( bool* )v; }
  };

  struct MetaConstCharStar : public MetaType
  {
    auto GetName() const -> const char* override                   { return "char*"; }
    auto GetSizeOf() const -> int override                         { return sizeof( char* ); }
    auto ToString( const void* v ) const -> String override        { return AsStr( v ); }
    auto ToNumber( const void* v ) const -> float override         { return ( float )Atoi( AsStr( v ) ); }
    void Cast( CastParams ) const override                         { TAC_ASSERT_INVALID_CODE_PATH; }
    void JsonSerialize( Json* json, const void* v ) const override { json->SetString( AsStr( v ) ); }
    void JsonDeserialize( const Json*, void* ) const override      { TAC_ASSERT_INVALID_CODE_PATH; }

  private:
    auto AsStr( const void* v ) const -> const char* { return ( const char* )v; }
  };

  struct MetaFloat : public MetaType
  {
    auto GetName() const -> const char* override               { return "float"; }
    auto GetSizeOf() const -> int override                     { return sizeof( float ); }
    auto ToString( const void* v ) const -> String override    { return Tac::ToString( AsFloatRef( v ) ); }
    auto ToNumber( const void* v ) const -> float override     { return AsFloatRef( v ); }
    void Cast( CastParams castParams ) const override
    {
      *( float* )castParams.mDst = ( float )castParams.mSrcType->ToNumber( castParams.mSrc );
    }
    void JsonSerialize( Json* json, const void* v ) const override
    {
      json->SetNumber( MetaCast< JsonNumber >( v, this ) );
    }
    void JsonDeserialize( const Json* json, void* v ) const override
    {
      const CastParams castParams
      {
        .mDst     { v },
        .mSrc     { TryGetJsonMetapointer( json ) },
        .mSrcType { TryGetJsonMetatype( json ) },
      };
      Cast( castParams );
    }
    bool Equals( const void* a, const void* b ) const override { return AsFloatRef( a ) == AsFloatRef( b ); }
    void Copy( CopyParams cp ) const override                  { AsFloatRef( cp.mDst ) = AsFloatRef( cp.mSrc ); }

  private:
    auto AsFloatRef( const void* v ) const -> const float&     { return *( float* )v; }
    auto AsFloatRef( dynmc void* v ) const -> dynmc float&     { return *( float* )v; }
  };

  struct MetaDouble : public MetaType
  {
    auto GetName() const -> const char* override               { return "double"; }
    auto GetSizeOf() const -> int override                     { return sizeof( double ); }
    auto ToString( const void* v ) const -> String override    { return Tac::ToString( AsDoubleRef( v ) ); }
    auto ToNumber( const void* v ) const -> float override     { return ( float )AsDoubleRef( v ); }
    void Cast( CastParams castParams ) const override
    {
      *( double* )castParams.mDst = ( double )castParams.mSrcType->ToNumber( castParams.mSrc );
    }
    void JsonSerialize( Json* json, const void* v ) const override
    {
      json->SetNumber( MetaCast< JsonNumber >( v, this ) );
    }
    void JsonDeserialize( const Json* json, void* v ) const override
    {
      const CastParams castParams
      {
        .mDst     { v },
        .mSrc     { TryGetJsonMetapointer( json ) },
        .mSrcType { TryGetJsonMetatype( json ) },
      };
      Cast( castParams );
    }
    bool Equals( const void* a, const void* b ) const override { return AsDoubleRef( a ) == AsDoubleRef( b ); }
    void Copy( CopyParams cp ) const override                  { AsDoubleRef( cp.mDst ) = AsDoubleRef( cp.mSrc ); }

  private:
    auto AsDoubleRef( const void* v ) const -> const double&   { return *( double* )v; }
    auto AsDoubleRef( dynmc void* v ) const -> dynmc double&   { return *( double* )v; }
  };

  struct MetaNull : public MetaType
  {
    auto GetName() const -> const char* override               { return "null"; }
    auto GetSizeOf() const -> int override                     { return 0; }
    auto ToString( const void* ) const -> String override      { return "null"; }
    auto ToNumber( const void* ) const -> float override       { return 0; }
    void Cast( CastParams ) const override                     { TAC_ASSERT_INVALID_CODE_PATH; }
    void JsonSerialize( Json*, const void* ) const override    { TAC_ASSERT_INVALID_CODE_PATH; }
    void JsonDeserialize( const Json*, void* ) const override  { TAC_ASSERT_INVALID_CODE_PATH; }
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

auto Tac::GetNullMetaType() -> const Tac::MetaType& { static const MetaNull sMetaNull; return sMetaNull; }

