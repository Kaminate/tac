#include "tac_string_meta.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/meta/tac_meta.h"

namespace Tac
{

  struct MetaString : public MetaType
  {
    const char* GetName() const override                                                            { return "String"; }
    int         GetSizeOf() const override                                                          { return sizeof( String ); }
    String      ToString( const void* s ) const override                                            { return AsStringRef( s ); }
    float       ToNumber( const void* s ) const override                                            { return Atof( AsStringRef( s ) ); }
    void        Cast( CastParams castParams ) const override                                        { AsStringRef( castParams.mDst ) = castParams.mSrcType->ToString( castParams.mSrc ); }
    void        JsonSerialize( Json* json, const void* s ) const override                           { json->SetString( AsStringRef( s ) ); }
    void        JsonDeserialize( const Json* json ,void* s ) const override                         { AsStringRef( s ) = json->mString; }
    int         ToInt( const void* s ) const                                                        { return Atoi( AsStringRef( s ) ); };
    bool        Equals( const void* a, const void* b ) const                                        { return AsStringRef( a ) == AsStringRef( b ); }
    void        Copy( CopyParams cp ) const                                                         { AsStringRef( cp.mDst ) = AsStringRef( cp.mSrc ); }

  private:
    static const String& AsStringRef( const void* s )                                               { return *( String* )s; }
    static dynmc String& AsStringRef( void* s )                                                     { return *( String* )s; }
  };

  TAC_META_IMPL_TYPE( String, MetaString );

} // namespace Tac


