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
    StringID( StringView );
    bool empty() const;
    auto GetValue() const -> HashValue;
    operator HashValue() const;
  private:
    void Init( StringView ) const;
    HashValue mHashedValue {};
  };

  bool operator <  ( StringID, StringID );
  bool operator == ( StringID, StringID );
  auto StringIDDebugLookup( StringID ) -> StringView;
}
