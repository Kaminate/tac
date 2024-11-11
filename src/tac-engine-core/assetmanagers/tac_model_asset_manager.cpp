#include "tac_model_asset_manager.h" // self-inc

#include "tac-engine-core/assetmanagers/gltf/tac_model_asset_loader_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_resident_model_file.h"
#include "tac-engine-core/assetmanagers/gltf/tac_model_load_synchronous.h"
#include "tac-engine-core/assetmanagers/obj/tac_model_asset_loader_obj.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager_backend.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/string/tac_string_identifier.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/job/tac_job_queue.h"
#include "tac-engine-core/assetmanagers/gltf/tac_gltf.h"


namespace Tac
{
  static Map< HashValue, Mesh* >   sMeshMap;

  static HashValue HashVertexDeclaration( const Render::VertexDeclaration& vertexDeclaration )
  {
    return Hash( ( HashValue )vertexDeclaration.mAlignedByteOffset,
                 ( HashValue )vertexDeclaration.mAttribute,
                 ( HashValue )vertexDeclaration.mFormat.mElementCount,
                 ( HashValue )vertexDeclaration.mFormat.mPerElementByteCount,
                 ( HashValue )vertexDeclaration.mFormat.mPerElementDataType );
  }

  static HashValue HashVertexDeclarations( const Render::VertexDeclarations& vertexDeclarations )
  {
    Hasher hasher;
    for( const Render::VertexDeclaration& vertexDeclaration : vertexDeclarations )
      hasher.Eat( HashVertexDeclaration( vertexDeclaration ) );

    return hasher;
  }

  static HashValue GetModelHash( const AssetPathStringView& path,
                                 const int iModel,
                                 const Render::VertexDeclarations& vtxDecls )
  {
    const HashValue hashPath{ Hash( ( StringView )path ) };
    const HashValue hashDecl{ HashVertexDeclarations( vtxDecls ) };

    Hasher hasher;
    hasher.Eat( hashPath );
    hasher.Eat( hashDecl );
    hasher.Eat( ( HashValue )iModel );
    return hasher;
  }

  static HashValue GetModelHash( const ModelAssetManager::Params& params)
  {
    const AssetPathStringView& path{ params.mPath };
    const int iModel{ params.mModelIndex };
    const Render::VertexDeclarations& vtxDecls{ params.mOptVtxDecls };
    const HashValue hashedValue{ GetModelHash( path, iModel , vtxDecls ) };
    return hashedValue;
  }


  // -----------------------------------------------------------------------------------------------
  
  Mesh* ModelAssetManager::GetMesh( Params params, Errors& errors )
  {
    if ( params.mPath.empty() )
      return nullptr;

    const HashValue hashedValue{ GetModelHash( params ) };

    if( Optional< Mesh* > mesh{ sMeshMap.FindVal( hashedValue ) } )
      return *mesh;

    const StringView pathExt{ params.mPath.GetFileExtension() };
    const MeshLoadFunction meshLoadFunction { ModelLoadFunctionFind( pathExt ) };
    if( !meshLoadFunction )
      return nullptr;

    Mesh* mesh { TAC_NEW Mesh };
    *mesh = TAC_CALL_RET( meshLoadFunction( params, errors ) );
    TAC_ASSERT( !mesh->mVertexDecls.empty() );

    sMeshMap[ hashedValue ] = mesh;

    return mesh;
  }

  void  ModelAssetManager::Init()
  {
    WavefrontObj::Init();
    GltfLoaderInit();
  }

  void  ModelAssetManager::Uninit()
  {
    for( auto pair : sMeshMap )
    {
      Mesh* mesh{ pair.mSecond };
      for( SubMesh& submesh : mesh->mSubMeshes )
      {
        submesh.ClearBuffers();
      }

      TAC_DELETE mesh;
    }

    sMeshMap = {};
  }


} // namespace Tac
