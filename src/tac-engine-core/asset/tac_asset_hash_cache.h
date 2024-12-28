#pragma once

#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/error/tac_error_handling.h"


namespace Tac
{
  struct AssetHash
  {
    AssetHash() = default;
    AssetHash( AssetPathStringView );

    HashValue mHashValue{};
  };

  inline bool operator == ( const AssetHash& a, const AssetHash& b ) { return a.mHashValue == b.mHashValue; }
  inline bool operator <  ( const AssetHash& a, const AssetHash& b ) { return a.mHashValue < b.mHashValue; }

  struct AssetHashCache
  {
    static void                Init( Errors& );
    static AssetPathStringView GetPathFromHash( AssetHash );
  };

} // namespace Tac
