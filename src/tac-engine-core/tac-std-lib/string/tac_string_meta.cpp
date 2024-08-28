#include "tac_string_meta.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"

namespace Tac
{

  struct StringMetaType : public MetaType
  {
    const char* GetName() const override;
    int         GetSizeOf() const override;
    String      ToString( const void* ) const override;
    float       ToNumber( const void* ) const override;
    void        Cast( CastParams ) const override;
    void        JsonSerialize( Json* , const void* ) const override;
    void        JsonDeserialize( const Json* , void* ) const override;
    int         ToInt( const void* ) const;

  private:
    static const String& AsStringRef( const void* s ) { return *( String* )s; }
    static dynmc String& AsStringRef( void* s )       { return *( String* )s; }
  };

  // -----------------------------------------------------------------------------------------------

  const char* StringMetaType::GetName() const { return "String"; }
  int         StringMetaType::GetSizeOf() const               { return sizeof( String ); }
  String      StringMetaType::ToString( const void* s ) const { return *( String* )s; }
  float       StringMetaType::ToNumber( const void* s ) const { return Atof( *( String* )s ); }
  void        StringMetaType::Cast( CastParams castParams ) const
  {
    AsStringRef( castParams.mDst ) = castParams.mSrcType->ToString( castParams.mSrc );
  }
  void        StringMetaType::JsonSerialize( Json* json, const void* s ) const
  {
    json->SetString( *( String* )s );
  }
  void        StringMetaType::JsonDeserialize( const Json* json, void* s ) const
  {
    AsStringRef( s ) = json->mString;
  }
  int         StringMetaType::ToInt( const void* s ) const
  {
    return Atoi( AsStringRef( s ) );
  }

  // -----------------------------------------------------------------------------------------------

  static StringMetaType sStringMetaType;
} // namespace Tac

const Tac::MetaType& Tac::GetMetaType( const String& ) { return sStringMetaType; }

