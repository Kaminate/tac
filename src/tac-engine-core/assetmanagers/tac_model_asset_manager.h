#pragma once

#include "tac-std-lib/error/tac_error_handling.h" // Errors
#include "tac-engine-core/asset/tac_asset.h" // AssetPathStringView
#include "tac-engine-core/assetmanagers/tac_mesh.h" // Mesh
#include "tac-rhi/render3/tac_render_api.h" // VertexDeclarations

namespace Tac
{
  struct ModelAssetManager
  {
    struct Params
    {
      //                         Path to a file on disk, such as a .gltf(preferred), .obj, or .fbx
      AssetPathStringView        mPath        {};

      //                         When the asset importer imports a model file, all of the
      //                         renderable objects in the file are linearized into an array, and
      //                         indexed by mModelIndex.
      int                        mModelIndex  {};

      //                         When getting a mesh, the asset manager automatically creates gpu
      //                         vertex buffers. If mOptVtxDecls is specified, those gpu buffers
      //                         will follow the speicified format. Otherwise, a default vertex
      //                         declaration will be created (and stored in Mesh::mVertexDecls).
      //                         Meshes are allowed to be loaded with multiple vertex declarations.
      Render::VertexDeclarations mOptVtxDecls {};
    };

    static void                    Init();
    static void                    Uninit();

    //                             Note: IBindlessArray::Bindings are inside each Mesh's SubMesh
    static Mesh*                   GetMesh( Params, Errors& );
    static Render::IBindlessArray* GetBindlessArray();
  };

} // namespace Tac
