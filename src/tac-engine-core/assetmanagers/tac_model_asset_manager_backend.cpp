#include "tac_model_asset_manager_backend.h" // self-inc

#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-std-lib/containers/tac_map.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"


namespace Tac
{
  static Map< HashValue, MeshLoadFunction > functionMap;

  static void ValidateExt( MeshFileExt ext )
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
}

void Tac::ModelLoadFunctionRegister( MeshLoadFunction meshLoadFunction,
                                     MeshFileExt ext )
{
  ValidateExt( ext );

  const HashValue hashedValue { ModelExtensionHash( ext ) };
  functionMap[ hashedValue ] = meshLoadFunction;
}

Tac::MeshLoadFunction Tac::ModelLoadFunctionFind( MeshFileExt ext )
{
  ValidateExt(ext);

  const HashValue hashedValue { ModelExtensionHash( ext ) };
  const Optional< MeshLoadFunction > fn = functionMap.FindVal( hashedValue );

  return fn.GetValueOr( nullptr );
}

