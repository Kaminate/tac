#include "tac_meta.h" // self-inc

#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/memory/tac_memory.h"
//#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_json.h"

import std; // fstream, iostream, iomanip, format
//#include <fstream>
//#include <iostream>
//#include <iomanip>
using std::size_t;

namespace Tac
{
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

  // -----------------------------------------------------------------------------------------------

  void        MetaType::Read( ReadStream*, dynmc void* )  const  { TAC_ASSERT_UNIMPLEMENTED; }
  void        MetaType::Write( WriteStream*, const void* ) const { TAC_ASSERT_UNIMPLEMENTED; }
  void        MetaType::Copy( CopyParams ) const                 { TAC_ASSERT_UNIMPLEMENTED; }

  bool        MetaType::Equals( const void*, const void* ) const
  {
    TAC_ASSERT_UNIMPLEMENTED;
    return false;
  }


  // -----------------------------------------------------------------------------------------------

  struct IntMetaType : public MetaType
  {
    const char* GetName() const override;
    int         GetSizeOf() const override;
    String      ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    int         ToInt( const void* ) const;
  };
  
  struct CharStarMetaType : public MetaType
  {
    const char* GetName() const override;
    int         GetSizeOf() const override;
    String      ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    
  };

  struct FloatMetaType : public MetaType
  {
    const char* GetName() const override;
    int         GetSizeOf() const override;
    String      ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json*, const void* ) const override;
    void        JsonDeserialize( const Json*, void* ) const override;
    float       ToFloat( const void* v ) const;
  };

  struct DoubleMetaType : public MetaType
  {
    const char* GetName() const override;
    int         GetSizeOf() const override;
    String      ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    double      ToDouble( const void* ) const;
    
  };

  struct NullMetaType : public MetaType
  {
    const char* GetName() const override;
    int         GetSizeOf() const override;
    String      ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json*, const void* ) const override;
    void        JsonDeserialize( const Json*, void* ) const override;

  };

  static const IntMetaType      sIntMetaType;
  static const CharStarMetaType sCharStarMetaType;
  static const FloatMetaType    sFloatMetaType;
  static const DoubleMetaType   sDoubleMetaType;
  static const NullMetaType     sNullMetaType;

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

  String      IntMetaType::ToString( const void* v ) const
  {
    return Tac::ToString( ToInt( v ) );
  }

  const char* IntMetaType::GetName() const  { return "int"; }

  int         IntMetaType::GetSizeOf() const  { return sizeof( int ); }

  float       IntMetaType::ToNumber( const void* v ) const
  {
    return ( float )ToInt( v );
  }

  void        IntMetaType::Cast( CastParams castParams ) const 
  {
    *( int* )castParams.mDst = ( int )castParams.mSrcType->ToNumber( castParams.mSrc );
  }

  void        IntMetaType::JsonSerialize( Json* json, const void* v ) const 
  {
    json->SetNumber( MetaCast< JsonNumber >( v, this ) );
  }

  void        IntMetaType::JsonDeserialize( const Json* json, void* v ) const 
  {
    const CastParams castParams
    {
      .mDst     { v},
      .mSrc     { TryGetJsonMetapointer( json )},
      .mSrcType { TryGetJsonMetatype( json )},
    };
    Cast( castParams );
  }

  // -----------------------------------------------------------------------------------------------

  const char* FloatMetaType::GetName() const { return "float"; }

  int         FloatMetaType::GetSizeOf() const { return sizeof( float ); }

  float       FloatMetaType::ToNumber( const void* v ) const { return ToFloat( v ); }

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

  String      FloatMetaType::ToString( const void* v ) const
  {
    return Tac::ToString( ToFloat( v ) );
  }

  float       FloatMetaType::ToFloat( const void* v ) const { return *( float* )v; }

  // -----------------------------------------------------------------------------------------------

  double      DoubleMetaType::ToDouble( const void* v ) const
  {
    const double d { *( ( const double* )v ) };
    return d;
  }

  String      DoubleMetaType::ToString( const void* v ) const
  {
    return Tac::ToString( ToDouble( v ) );
  }

  const char* DoubleMetaType::GetName() const { return "double"; }

  int         DoubleMetaType::GetSizeOf() const { return sizeof( double ); }

  float       DoubleMetaType::ToNumber( const void* v ) const 
  {
    return ( float )ToDouble( v );
  }

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
      .mDst     { v},
      .mSrc     { TryGetJsonMetapointer( json )},
      .mSrcType { TryGetJsonMetatype( json )},
    };
    Cast( castParams );
  }

  // -----------------------------------------------------------------------------------------------

  const char* NullMetaType::GetName() const              { return "null"; }

  int         NullMetaType::GetSizeOf() const            { return 0; }

  String      NullMetaType::ToString( const void* v ) const    { return "null"; }

  float       NullMetaType::ToNumber( const void* v ) const    { return 0; }

  void        NullMetaType::Cast( CastParams ) const 
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

  int         CharStarMetaType::GetSizeOf() const  { return sizeof( char* ); }

  String      CharStarMetaType::ToString( const void* v ) const  { return *( const char** )v; }

  float       CharStarMetaType::ToNumber( const void* v ) const  { return ( float )Atoi( *( const char** )v ); }

  void        CharStarMetaType::Cast( CastParams ) const 
  {
    //*( const char** )dst = srcType->ToString( src );
    TAC_ASSERT_INVALID_CODE_PATH;
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

