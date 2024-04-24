#include "tac_model_asset_manager.h" // self-inc

#include "tac-engine-core/assetmanagers/gltf/tac_model_asset_loader_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_resident_model_file.h"
#include "tac-engine-core/assetmanagers/gltf/tac_model_load_synchronous.h"
#include "tac-engine-core/assetmanagers/obj/tac_model_asset_loader_obj.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager_backend.h"
//#include "tac-rhi/render3/tac_render_api.h"
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
  static Map< StringID, Mesh* >    mMeshes;
  static Map< HashValue, Mesh* >   sTryNewTHingMeshes;

  static HashValue HashVertexDeclaration( const Render::VertexDeclaration& vertexDeclaration )
  {
    return Hash( ( HashValue )vertexDeclaration.mAlignedByteOffset,
                 ( HashValue )vertexDeclaration.mAttribute,
                 ( HashValue )vertexDeclaration.mTextureFormat.mElementCount,
                 ( HashValue )vertexDeclaration.mTextureFormat.mPerElementByteCount,
                 ( HashValue )vertexDeclaration.mTextureFormat.mPerElementDataType );
  }

  static HashValue HashVertexDeclarations( const Render::VertexDeclarations& vertexDeclarations )
  {
    Hasher hasher;
    for( const Render::VertexDeclaration& vertexDeclaration : vertexDeclarations )
      hasher.Eat( HashVertexDeclaration( vertexDeclaration ) );
    return hasher;
  }

  Mesh* ModelAssetManagerGetMeshTryingNewThing( const AssetPathStringView& path,
                                                const int iModel,
                                                const Render::VertexDeclarations& vtxDecls,
                                                Errors& errors )
  {
    if(path.empty())
      return nullptr;

    const HashValue hashedValue = Hash( Hash( (StringView)path ),
                                        HashVertexDeclarations( vtxDecls ),
                                        ( HashValue )iModel );

    if( Optional< Mesh* > mesh = sTryNewTHingMeshes.FindVal( hashedValue ) )
      return *mesh;

    const StringView pathExt = path.GetFileExtension();
    MeshLoadFunction meshLoadFunction = ModelLoadFunctionFind( pathExt );
    if( !meshLoadFunction )
      return nullptr;

    auto mesh = TAC_NEW Mesh;
    *mesh = TAC_CALL_RET( {}, meshLoadFunction( path, iModel, vtxDecls, errors ) );

    sTryNewTHingMeshes[ hashedValue ] = mesh;

    return mesh;
  }


  void  ModelAssetManagerInit()
  {
    WavefrontObjLoaderInit();
    GltfLoaderInit();
  }

  void  ModelAssetManagerUninit()
  {
    for( auto [sid, mesh] : mMeshes )
    {
      for( SubMesh& submesh : mesh->mSubMeshes )
      {
        Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
        renderDevice->DestroyBuffer( submesh.mIndexBuffer );
        renderDevice->DestroyBuffer( submesh.mVertexBuffer );
      }
    }
  }


} // namespace Tac
