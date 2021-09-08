#pragma once

#include "src/common/tacHash.h"

namespace Tac
{
  struct StringView;
  struct StringID
  {
    StringID( const char* );
    StringID( const char*, int );
    StringID( const StringView& );
    operator HashedValue() const;
    HashedValue mHashedValue = 0;
  };

  bool          operator <  ( StringID, StringID );
  bool          operator == ( StringID, StringID );
  const char*   StringIDDebugLookup( StringID );
}
