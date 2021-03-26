#include "src/common/string/tacStringIdentifier.h"
#include "src/common/string/tacString.h"
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  const int   kMaxStringDictionaryEntries = 1024;
  String      gStringLookup[ kMaxStringDictionaryEntries ];

  const char* StringIDDebugLookup( StringID stringID )
  {
    if( !IsDebugMode() )
      return "";
    return gStringLookup[ stringID.mHashedValue % kMaxStringDictionaryEntries ];
  }

  static void DebugSetStringLookup( StringID stringID, StringView stringView )
  {
    if( !IsDebugMode() )
      return;
    String& lookupEntry = gStringLookup[ stringID.mHashedValue % kMaxStringDictionaryEntries ];
    if( !lookupEntry.empty() )
      return;
    lookupEntry = stringView;
  }

  StringID::StringID( const char* s, int n )
  {
    mHashedValue = Tac::HashAddBytes( s, n );
    TAC_ASSERT( mHashedValue );
    DebugSetStringLookup( *this, StringView( s, n ) );
  }
  StringID::StringID( const char* s ) : StringID( StringView( s ) ) {}
  StringID::StringID( const StringView& stringView ) : StringID( stringView.data(), stringView.size() ) {}

  bool operator <  ( StringID a, StringID b ) { return a.mHashedValue < b.mHashedValue; }
  bool operator == ( StringID a, StringID b ) { return a.mHashedValue == b.mHashedValue; }
}
