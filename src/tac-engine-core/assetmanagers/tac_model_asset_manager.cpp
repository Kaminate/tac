#include "tac_model_asset_manager.h" // self-inc

#include "tac-engine-core/assetmanagers/gltf/tac_model_asset_loader_gltf.h"
#include "tac-engine-core/assetmanagers/gltf/tac_resident_model_file.h"
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
  static Render::IBindlessArray*   sModelBindlessArray;

  static auto HashVertexDeclaration( const Render::VertexDeclaration& vertexDeclaration ) -> HashValue
  {
    return Hash( ( HashValue )vertexDeclaration.mAlignedByteOffset,
                 ( HashValue )vertexDeclaration.mAttribute,
                 ( HashValue )vertexDeclaration.mFormat.mElementCount,
                 ( HashValue )vertexDeclaration.mFormat.mPerElementByteCount,
                 ( HashValue )vertexDeclaration.mFormat.mPerElementDataType );
  }

  static auto HashVertexDeclarations( const Render::VertexDeclarations& vertexDeclarations ) -> HashValue
  {
    Hasher hasher;
    for( const Render::VertexDeclaration& vertexDeclaration : vertexDeclarations )
      hasher.Eat( HashVertexDeclaration( vertexDeclaration ) );

    return hasher;
  }

  static auto GetModelHash( const AssetPathStringView& path,
                                 const int iModel,
                                 const Render::VertexDeclarations& vtxDecls ) -> HashValue
  {
    const HashValue hashPath{ Hash( ( StringView )path ) };
    const HashValue hashDecl{ HashVertexDeclarations( vtxDecls ) };

    Hasher hasher;
    hasher.Eat( hashPath );
    hasher.Eat( hashDecl );
    hasher.Eat( ( HashValue )iModel );
    return hasher;
  }

  static auto GetModelHash( const ModelAssetManager::Params& params ) -> HashValue
  {
    const AssetPathStringView& path{ params.mPath };
    const int iModel{ params.mModelIndex };
    const Render::VertexDeclarations& vtxDecls{ params.mOptVtxDecls };
    const HashValue hashedValue{ GetModelHash( path, iModel , vtxDecls ) };
    return hashedValue;
  }

  static void BindlessModelArrayInit()
  {
    const Render::IBindlessArray::Params bindlessArrayParams
    {
      .mHandleType { Render::HandleType::kBuffer },
      .mBinding    { Render::Binding::ShaderResource },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    sModelBindlessArray = renderDevice->CreateBindlessArray( bindlessArrayParams );
  }

  static void FileFormatLoadersInit()
  {
    WavefrontObj::Init();
    GltfLoaderInit();
  }

  // -----------------------------------------------------------------------------------------------
  
  auto ModelAssetManager::GetMesh( Params params, Errors& errors ) -> Mesh*
  {
    if ( params.mPath.empty() )
      return nullptr;

    const HashValue hashedValue{ GetModelHash( params ) };

    if( auto it { sMeshMap.find( hashedValue ) }; it != sMeshMap.end() )
    {
      auto& [_, mesh] { *it};
      return mesh;
    }

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

  auto ModelAssetManager::GetBindlessArray() -> Render::IBindlessArray* { return sModelBindlessArray; }

  void ModelAssetManager::Init()
  {
    FileFormatLoadersInit();
    BindlessModelArrayInit();
  }

  void ModelAssetManager::Uninit()
  {
    for( auto& [_, mesh ] : sMeshMap )
    {
      for( SubMesh& submesh : mesh->mSubMeshes )
      {
        submesh.ClearBuffers();
      }

      TAC_DELETE mesh;
    }

    sMeshMap = {};
  }


} // namespace Tac
