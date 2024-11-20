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
      AssetPathStringView        mPath        {};
      int                        mModelIndex  {};
      Render::VertexDeclarations mOptVtxDecls {};
    };

    static void                    Init();
    static void                    Uninit();

    //                             Note: IBindlessArray::Bindings are inside each Mesh's SubMesh
    static Mesh*                   GetMesh( Params, Errors& );
    static Render::IBindlessArray* GetBindlessArray();
  };

} // namespace Tac
