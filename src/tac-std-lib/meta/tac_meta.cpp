#include "tac_meta.h" // self-inc

#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/memory/tac_memory.h"
//#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_json.h"

import std; // fstream, iostream, iomanip, format
//#include <fstream>
//#include <iostream>
//#include <iomanip>
using std::size_t;

namespace Tac
{
  //const char* MetaType::GetName() const          { return 0; }
  //size_t      MetaType::GetSizeOf() const        { return 0; }
  //const char* MetaType::ToString( void* ) const  { return 0; }
  //float       MetaType::ToNumber( void* ) const  { return 0; }
  //void        MetaType::Cast( void*, void*, const MetaType* ) const {}



  // How should we handle failure?
  static const void* TryGetJsonMetapointer( const Json* json )
  {
    switch( json->mType )
    {
      case JsonType::Number: return &json->mNumber;
      case JsonType::Bool: return &json->mBoolean;
      case JsonType::String: return &json->mString;
      default: return nullptr;
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

  struct IntMetaType : public MetaType
  {
    const char* GetName() const override;
    size_t      GetSizeOf() const override;
    const char* ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( void* dst, const void* src, const MetaType* srcType ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    int         ToInt( const void* ) const;
  };
  

  struct CharStarMetaType : public MetaType
  {
    const char* GetName() const override;
    size_t      GetSizeOf() const override;
    const char* ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( void* dst, const void* src, const MetaType* srcType ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    
  };


  struct FloatMetaType : public MetaType
  {
    const char* GetName() const override;
    size_t      GetSizeOf() const override;
    const char* ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( void* dst, const void* src, const MetaType* srcType ) const override;
    void        JsonSerialize( Json*, const void* ) const override;
    void        JsonDeserialize( const Json*, void* ) const override;
    float       ToFloat( const void* v ) const;
  };


  struct DoubleMetaType : public MetaType
  {
    const char* GetName() const override;
    size_t      GetSizeOf() const override;
    const char* ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( void* dst, const void* src, const MetaType* srcType ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    double      ToDouble( const void* ) const;
    
  };


  struct NullMetaType : public MetaType
  {
    const char* GetName() const override;
    size_t      GetSizeOf() const override;
    const char* ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( void* dst, const void* src, const MetaType* srcType ) const override;
    void        JsonSerialize( Json*, const void* ) const override;
    void        JsonDeserialize( const Json*, void* ) const override;

  };

  static IntMetaType sIntMetaType;
  static CharStarMetaType sCharStarMetaType;
  static FloatMetaType sFloatMetaType;
  static DoubleMetaType sDoubleMetaType;
  static NullMetaType sNullMetaType;

  const MetaType& GetMetaType( const int& )    { return sIntMetaType; }
  const MetaType& GetMetaType( const float& )  { return sFloatMetaType; }
  const MetaType& GetMetaType( const double& ) { return sDoubleMetaType; }
  const MetaType& GetMetaType( const char*& )  { return sCharStarMetaType; }
  const MetaType& GetNullMetaType()            { return sNullMetaType; }

  // -----------------------------------------------------------------------------------------------

  int         IntMetaType::ToInt( const void* v ) const
  {
    return   *( const int* )v;
  }

  const char* IntMetaType::ToString( const void* v ) const
  {
    return Tac::ToString( ToInt( v ) );
  }

  const char* IntMetaType::GetName() const  { return "int"; }
  size_t      IntMetaType::GetSizeOf() const  { return sizeof( int ); }
  float       IntMetaType::ToNumber( const void* v ) const
  {
    return ( float )ToInt( v );
  }
  void        IntMetaType::Cast( void* dst, const void* src, const MetaType* srcType ) const 
  {
    *( int* )dst = ( int )srcType->ToNumber( src );
  }
  void        IntMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    json->SetNumber( MetaCast< JsonNumber >( v, this ) );
  }
  void        IntMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    Cast( v, TryGetJsonMetapointer( json ), TryGetJsonMetatype( json ) );
  }

  // -----------------------------------------------------------------------------------------------
  const char* FloatMetaType::GetName() const { return "float"; }
  size_t      FloatMetaType::GetSizeOf() const { return sizeof( float ); }

  float       FloatMetaType::ToNumber( const void* v ) const { return ToFloat( v ); }

  void        FloatMetaType::Cast( void* dst, const void* src, const MetaType* srcType ) const 
  {
    *( float* )dst = ( float )srcType->ToNumber( src );
  }

  void        FloatMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    json->SetNumber( MetaCast< JsonNumber >( v, this ) );
  }

  void        FloatMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    Cast( v, TryGetJsonMetapointer( json ), TryGetJsonMetatype( json ) );
  }

  const char* FloatMetaType::ToString( const void* v ) const
  {
    return Tac::ToString( ToFloat( v ) );
  }

  float       FloatMetaType::ToFloat( const void* v ) const { return *( float* )v; }

  // -----------------------------------------------------------------------------------------------

  double      DoubleMetaType::ToDouble( const void* v ) const
  {
    const double d = *( ( const double* )v );
    return d;
  }
  const char* DoubleMetaType::ToString( const void* v ) const
  {
    return Tac::ToString( ToDouble( v ) );
  }
  const char* DoubleMetaType::GetName() const { return "double"; }
  size_t      DoubleMetaType::GetSizeOf() const { return sizeof( double ); }
  float       DoubleMetaType::ToNumber( const void* v ) const 
  {
    return ( float )ToDouble( v );
  }
  void        DoubleMetaType::Cast( void* dst, const void* src, const MetaType* srcType ) const 
  {
    *( double* )dst = ( double )srcType->ToNumber( src );
  }
  void        DoubleMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    json->SetNumber( MetaCast< JsonNumber >( v, this ) );
  }
  void        DoubleMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    Cast( v, TryGetJsonMetapointer( json ), TryGetJsonMetatype( json ) );
  }

  // -----------------------------------------------------------------------------------------------

  const char* NullMetaType::GetName() const              { return "null"; }
  size_t      NullMetaType::GetSizeOf() const            { return 0; }
  const char* NullMetaType::ToString( const void* v ) const    { return "null"; }
  float       NullMetaType::ToNumber( const void* v ) const    { return 0; }
  void        NullMetaType::Cast( void* dst, const void* src, const MetaType* srcType ) const 
  {
    TAC_ASSERT_INVALID_CODE_PATH;
  }
  void        NullMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    TAC_ASSERT_INVALID_CODE_PATH;
  }
  void        NullMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    TAC_ASSERT_INVALID_CODE_PATH;
  }

  // -----------------------------------------------------------------------------------------------

  const char* CharStarMetaType::GetName() const  { return "char*"; }
  size_t      CharStarMetaType::GetSizeOf() const  { return sizeof( char* ); }
  const char* CharStarMetaType::ToString( const void* v ) const  { return *( const char** )v; }
  float       CharStarMetaType::ToNumber( const void* v ) const  { return ( float )Atoi( *( const char** )v ); }
  void        CharStarMetaType::Cast( void* dst, const void* src, const MetaType* srcType ) const 
  {
    *( const char** )dst = srcType->ToString( src );
  }
  void        CharStarMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    json->SetString( ToString( v ) );
  }
  void        CharStarMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    // like what are we supposed to do here?
    // you can serialize a Tac::String, but a charstar?
    TAC_ASSERT_INVALID_CODE_PATH;
  }
  // -----------------------------------------------------------------------------------------------

} // namespace Tac

