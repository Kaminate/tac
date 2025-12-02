#include "tac_string_identifier.h" // self-inc

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------
  static const int   kMaxStringDictionaryEntries{ 1024 };
  static String      gStringLookup[ kMaxStringDictionaryEntries ];
  // -----------------------------------------------------------------------------------------------

  // static functions

  static void DebugSetStringLookup( StringID stringID, const StringView stringView )
  {
    if constexpr( kIsDebugMode )
    {
      String& lookupEntry = gStringLookup[ stringID % kMaxStringDictionaryEntries ];
      if( !lookupEntry.empty() )
        return;

      lookupEntry = stringView;
    }
  }

  // -----------------------------------------------------------------------------------------------

  StringID::StringID( const char* s, int n ) : mHashedValue( Hash( s, n  ) )
  {
    Init( StringView( s, n ) );
  }

  StringID::StringID( const char* s ) : mHashedValue( Hash( s ) )
  {
    Init( s );
  }

  StringID::StringID( const StringView stringView ) : mHashedValue( Hash( stringView ) )
  {
    Init( stringView );
  }

  bool StringID::empty() const                  { return !mHashedValue; }
  auto StringID::GetValue() const  -> HashValue { return mHashedValue;  }

  void StringID::Init( const StringView s ) const
  {
    TAC_ASSERT( !empty() );
    DebugSetStringLookup( *this, s );
  }

  StringID::operator HashValue() const { return mHashedValue;  }

} // namespace Tac

bool Tac::operator <  ( StringID a, StringID b ) { return ( HashValue )a < ( HashValue )b; }
bool Tac::operator == ( StringID a, StringID b ) { return ( HashValue )a == ( HashValue )b; }

auto Tac::StringIDDebugLookup( const StringID stringID ) -> StringView
{
  if constexpr( kIsDebugMode )
    return "";

  return gStringLookup[ stringID % kMaxStringDictionaryEntries ];
}
