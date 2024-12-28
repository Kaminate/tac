#include "tac_asset_hash_cache.h"

#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/shell/tac_shell.h"

#include "tac-std-lib/containers/tac_map.h"


namespace Tac
{
  template<>
  HashValue Hash< >( AssetHash sv )
  {
    return sv.mHashValue;
  }
}


namespace Tac
{

  //static Map< HashValue, AssetPathString > sAssetHashCacheMap;
  static Map< AssetHash, AssetPathString > sAssetHashCacheMap;

  // -----------------------------------------------------------------------------------------------

  AssetHash::AssetHash( AssetPathStringView sv )
  {
    mHashValue = Hash( sv.data(), sv.size() );
  }

  // -----------------------------------------------------------------------------------------------

  void                AssetHashCache::Init( Errors& errors )
  {
    static bool sInitialized;
    if( sInitialized )
      return;


    const FileSys::Paths paths{
      FileSys::IterateFiles( "assets", FileSys::IterateType::Recursive, errors ) };

    for( const FileSys::Path path : paths )
    {
      const AssetPathStringView assetPath{ ModifyPathRelative( path, errors ) };
      const AssetHash assetHash{ assetPath };
      TAC_ASSERT( !sAssetHashCacheMap.contains(assetHash) );
      sAssetHashCacheMap[ assetHash ] = assetPath;
    }

    sInitialized = true;
  }

  AssetPathStringView AssetHashCache::GetPathFromHash( AssetHash assetHash )
  {
    return sAssetHashCacheMap[ assetHash ];
    //return sAssetHashCacheMap[ assetHash.mHashValue ];
  }

} // namespace Tac
