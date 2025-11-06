#include "tac_asset_hash_cache.h"

#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/shell/tac_shell.h"

#include "tac-std-lib/containers/tac_map.h"


namespace Tac
{
  template<> HashValue Hash<>( AssetHash sv ) { return sv.mHashValue; }

  static Map< AssetHash, AssetPathString > sAssetHashCacheMap;

  // -----------------------------------------------------------------------------------------------

  AssetHash::AssetHash( AssetPathStringView sv )
  {
    mHashValue = Hash( sv.data(), sv.size() );
  }

  // -----------------------------------------------------------------------------------------------

  void AssetHashCache::Init( Errors& errors )
  {
    static bool sInitialized;
    if( sInitialized )
      return;

    TAC_CALL( const AssetPathStrings paths{
      IterateAssetsInDir( "assets", AssetIterateType::Recursive, errors ) } );

    for( const AssetPathString& assetPath : paths )
    {
      //const AssetPathStringView assetPath{ ModifyPathRelative( path, errors ) };
      const AssetHash assetHash{ assetPath };
      TAC_ASSERT( !sAssetHashCacheMap.contains(assetHash) );
      sAssetHashCacheMap[ assetHash ] = assetPath;
    }

    sInitialized = true;
  }

  auto AssetHashCache::GetPathFromHash( AssetHash assetHash ) -> AssetPathStringView
  {
    return sAssetHashCacheMap[ assetHash ];
  }

} // namespace Tac
