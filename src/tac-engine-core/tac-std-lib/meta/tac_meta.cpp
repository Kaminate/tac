#include "tac_meta.h" // self-inc

#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/meta/tac_meta_integral.h"

namespace Tac
{
  // How should we handle failure?
  static const void* TryGetJsonMetapointer( const Json* json )
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
  static const MetaType* TryGetJsonMetatype( const Json* json )
  {
    switch( json->mType )
    {
      case JsonType::Number: return &GetMetaType< decltype( json->mNumber ) >();
      case JsonType::Bool: return &GetMetaType< decltype( json->mBoolean ) >();
      default: return nullptr;
    }
  }

  // -----------------------------------------------------------------------------------------------



  // -----------------------------------------------------------------------------------------------

  struct MetaBool : public MetaType
  {
    const char* GetName() const override                                                            { return "bool"; }
    int         GetSizeOf() const override                                                          { return sizeof( bool ); }
    String      ToString( const void* v ) const override                                            { return AsBool( v ) ? "true" : "false"; }
    float       ToNumber( const void* v ) const override                                            { return AsBool( v ) ? 1.0f : 0.0f; }
    void        Cast( CastParams castParams ) const override                                        { AsBool( castParams.mDst ) = ( bool )castParams.mSrcType->ToNumber( castParams.mSrc ); }
    void        JsonSerialize( Json* json, const void* v ) const override                           { json->SetBool( AsBool( v ) ); }
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
    bool        Equals( const void* a, const void* b ) const override                               { return AsBool( a ) == AsBool( b ); }
    void        Copy( CopyParams cp ) const override                                                { AsBool( cp.mDst ) = AsBool( cp.mSrc ); }

  private:
    const char* mName;
    const bool&    AsBool( const void* v ) const                                                    { return *( bool* )v; }
    dynmc bool&    AsBool( dynmc void* v ) const                                                    { return *( bool* )v; }
  };

  static MetaBool sMetaBool;
  TAC_META_IMPL_INSTANCE( bool, sMetaBool );


  static const MetaIntegralType< i8 >  sMeta_i8( "i8" );
  static const MetaIntegralType< i16 > sMeta_i16("16");
  static const MetaIntegralType< i32 > sMeta_i32("32");
  static const MetaIntegralType< i64 > sMeta_i64("64");
  TAC_META_IMPL_INSTANCE( i8,  sMeta_i8 );
  TAC_META_IMPL_INSTANCE( i16, sMeta_i16 );
  TAC_META_IMPL_INSTANCE( i32, sMeta_i32 );
  TAC_META_IMPL_INSTANCE( i64, sMeta_i64 );

  static const MetaIntegralType< u8 >  sMeta_u8( "u8" );
  static const MetaIntegralType< u16 > sMeta_u16( "u16" );
  static const MetaIntegralType< u32 > sMeta_u32( "u32" );
  static const MetaIntegralType< u64 > sMeta_u64( "u64" );
  TAC_META_IMPL_INSTANCE( u8,  sMeta_u8 );
  TAC_META_IMPL_INSTANCE( u16, sMeta_u8 );
  TAC_META_IMPL_INSTANCE( u32, sMeta_u8 );
  TAC_META_IMPL_INSTANCE( u64, sMeta_u8 );

  // -----------------------------------------------------------------------------------------------

  
  struct CharStarMetaType : public MetaType
  {
    const char* GetName() const override                                                            { return "char*"; }
    int         GetSizeOf() const override                                                          { return sizeof( char* ); }
    String      ToString( const void* v ) const override                                            { return AsStr( v ); }
    float       ToNumber( const void* v ) const override                                            { return ( float )Atoi( AsStr( v ) ); }
    void        Cast( CastParams ) const override                                                   { TAC_ASSERT_INVALID_CODE_PATH; }
    void        JsonSerialize( Json* json, const void* v) const override                            { json->SetString( AsStr( v ) ); }
    void        JsonDeserialize( const Json*, void* ) const override                                { TAC_ASSERT_INVALID_CODE_PATH; }

  private:
    const char* AsStr( const void* v ) const                                                        { return ( const char* )v; }
  };

  struct FloatMetaType : public MetaType
  {
    const char* GetName() const override                                                            { return "float"; }
    int         GetSizeOf() const override                                                          { return sizeof( float ); }
    String      ToString( const void* v ) const override                                            { return Tac::ToString( AsFloatRef( v ) ); }
    float       ToNumber( const void* v ) const override                                            { return AsFloatRef( v ); }
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json*, const void* ) const override;
    void        JsonDeserialize( const Json*, void* ) const override;
    bool        Equals( const void* a, const void* b ) const override                               { return AsFloatRef( a ) == AsFloatRef( b ); }
    void        Copy( CopyParams cp ) const override                                                { AsFloatRef( cp.mDst ) = AsFloatRef( cp.mSrc ); }

  private:
    const float&      AsFloatRef( const void* v ) const                                             { return *( float* )v; }
    dynmc float&      AsFloatRef( dynmc void* v ) const                                             { return *( float* )v; }
  };

  struct DoubleMetaType : public MetaType
  {
    const char* GetName() const override                                                            { return "double"; }
    int         GetSizeOf() const override                                                          { return sizeof( double ); }
    String      ToString( const void* v ) const override                                            { return Tac::ToString( AsDoubleRef( v ) ); }
    float       ToNumber( const void* v ) const override                                            { return ( float )AsDoubleRef( v ); }
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    bool        Equals( const void* a, const void* b ) const override                               { return AsDoubleRef( a ) == AsDoubleRef( b ); }
    void        Copy( CopyParams cp ) const override                                                { AsDoubleRef( cp.mDst ) = AsDoubleRef( cp.mSrc ); }

  private:
    const double&     AsDoubleRef( const void* v ) const                                            { return *( double* )v; }
    dynmc double&     AsDoubleRef( dynmc void* v ) const                                            { return *( double* )v; }
  };

  struct NullMetaType : public MetaType
  {
    const char* GetName() const override                                                            { return "null"; }
    int         GetSizeOf() const override                                                          { return 0; }
    String      ToString( const void* ) const override                                              { return "null"; }
    float       ToNumber( const void* ) const override                                              { return 0; }
    void        Cast( CastParams ) const override                                                   { TAC_ASSERT_INVALID_CODE_PATH; }
    void        JsonSerialize( Json*, const void* ) const override                                  { TAC_ASSERT_INVALID_CODE_PATH; }
    void        JsonDeserialize( const Json*, void* ) const override                                { TAC_ASSERT_INVALID_CODE_PATH; } 
  };

  //static const IntMetaType      sIntMetaType;
  static const CharStarMetaType sCharStarMetaType;
  static const FloatMetaType    sFloatMetaType;
  static const DoubleMetaType   sDoubleMetaType;
  static const NullMetaType     sNullMetaType;

  //const MetaType& GetMetaType( const int& )    { return sIntMetaType; }
  const MetaType& GetMetaType( const float& )  { return sFloatMetaType; }
  const MetaType& GetMetaType( const double& ) { return sDoubleMetaType; }
  const MetaType& GetMetaType( const char*& )  { return sCharStarMetaType; }
  const MetaType& GetNullMetaType()            { return sNullMetaType; }

  // -----------------------------------------------------------------------------------------------

  // -----------------------------------------------------------------------------------------------


  void        FloatMetaType::Cast( CastParams castParams ) const 
  {
    *( float* )castParams.mDst = ( float )castParams.mSrcType->ToNumber( castParams.mSrc );
  }

  void        FloatMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    json->SetNumber( MetaCast< JsonNumber >( v, this ) );
  }

  void        FloatMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    const CastParams castParams
    {
      .mDst     { v },
      .mSrc     { TryGetJsonMetapointer( json ) },
      .mSrcType { TryGetJsonMetatype( json ) },
    };
    Cast( castParams );
  }



  // -----------------------------------------------------------------------------------------------



  void        DoubleMetaType::Cast( CastParams castParams ) const 
  {
    *( double* )castParams.mDst = ( double )castParams.mSrcType->ToNumber( castParams.mSrc );
  }

  void        DoubleMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    json->SetNumber( MetaCast< JsonNumber >( v, this ) );
  }

  void        DoubleMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    const CastParams castParams
    {
      .mDst     { v },
      .mSrc     { TryGetJsonMetapointer( json ) },
      .mSrcType { TryGetJsonMetatype( json ) },
    };
    Cast( castParams );
  }

  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------


  // -----------------------------------------------------------------------------------------------

} // namespace Tac

