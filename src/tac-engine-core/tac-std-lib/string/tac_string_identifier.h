#pragma once

#include "tac-std-lib/dataprocess/tac_hash.h"

namespace Tac
{
  struct StringView;

  struct StringID
  {
    StringID() = default;
    StringID( const char* );
    StringID( const char*, int );
    StringID( const StringView& );
    bool empty() const;
    operator HashValue() const;
    HashValue GetValue() const;
  private:
    void Init( const StringView& ) const;

    HashValue mHashedValue {};
  };

  bool          operator <  ( StringID, StringID );
  bool          operator == ( StringID, StringID );
  StringView    StringIDDebugLookup( StringID );
}
