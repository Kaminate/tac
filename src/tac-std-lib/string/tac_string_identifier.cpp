#include "tac_string_identifier.h" // self-inc

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  const int   kMaxStringDictionaryEntries = 1024;
  String      gStringLookup[ kMaxStringDictionaryEntries ];
  // -----------------------------------------------------------------------------------------------

  // static functions

  static void DebugSetStringLookup( StringID stringID, const StringView& stringView )
  {
    if constexpr( !IsDebugMode )
      return;

    String& lookupEntry = gStringLookup[ stringID % kMaxStringDictionaryEntries ];
    if( !lookupEntry.empty() )
      return;

    lookupEntry = stringView;
  }

  // -----------------------------------------------------------------------------------------------

  // constructors

  StringID::StringID( const char* s, int n ) : mHashedValue( Hash( s, n  ) )
  {
    Init( StringView( s, n ) );
  }

  StringID::StringID( const char* s ) : mHashedValue( Hash( s ) )
  {
    Init( s );
  }

  StringID::StringID( const StringView& stringView ) : mHashedValue( Hash( stringView ) )
  {
    Init( stringView );
  }

  // -----------------------------------------------------------------------------------------------

  // public member functions

  bool      StringID::empty() const    { return !mHashedValue; }
  HashValue StringID::GetValue() const { return mHashedValue;  }

  // -----------------------------------------------------------------------------------------------

  // public member operators

  StringID::operator HashValue() const { return mHashedValue;  }

  // -----------------------------------------------------------------------------------------------

  // private member functions

  void StringID::Init( const StringView& s ) const
  {
    TAC_ASSERT( !empty() );
    DebugSetStringLookup( *this, s );
  }

  // -----------------------------------------------------------------------------------------------



  bool operator <  ( StringID a, StringID b ) { return ( HashValue )a < ( HashValue )b; }
  bool operator == ( StringID a, StringID b ) { return ( HashValue )a == ( HashValue )b; }

  // -----------------------------------------------------------------------------------------------

  StringView StringIDDebugLookup( const StringID stringID )
  {
    if constexpr( IsDebugMode )
      return "";

    return gStringLookup[ stringID % kMaxStringDictionaryEntries ];
  }

} // namespace Tac
