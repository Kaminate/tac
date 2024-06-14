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
    bool operator == ( const AssetHash& ) const = default;

    HashValue mHashValue{};
  };

  struct AssetHashCache
  {
    static void                Init( Errors& );
    static AssetPathStringView GetPathFromHash( AssetHash );
  };

} // namespace Tac
