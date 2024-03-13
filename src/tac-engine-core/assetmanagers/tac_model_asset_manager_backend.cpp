#include "tac-std-lib/assetmanagers/tac_model_asset_manager_backend.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/assetmanagers/tac_mesh.h"
#include "tac-std-lib/containers/tac_map.h"


namespace Tac
{
  static Map< HashValue, MeshLoadFunction* > functionMap;

  void ValidateExt( MeshFileExt ext )
  {
    if constexpr( IsDebugMode )
    {
      TAC_ASSERT( ext.size() > 1 && ext.front() == '.' );
    }
  }
  
  static HashValue ModelExtensionHash( MeshFileExt ext )
  {
    Hasher hasher;
    for( char c : ext )
      hasher.Eat( ToLower( c ) );
    return hasher;
  }

  void               ModelLoadFunctionRegister( MeshLoadFunction* meshLoadFunction,
                                                MeshFileExt ext )
  {
    ValidateExt( ext );

    const HashValue hashedValue = ModelExtensionHash( ext );
    functionMap[ hashedValue ] = meshLoadFunction;
  }

  MeshLoadFunction*  ModelLoadFunctionFind( MeshFileExt ext )
  {
    ValidateExt(ext);

    const HashValue hashedValue = ModelExtensionHash( ext );
    const Optional< MeshLoadFunction* > fn = functionMap.FindVal( hashedValue );

    return fn.GetValueOr( nullptr );
  }
}

