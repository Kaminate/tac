#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h" // AssetPathStringView
#include "tac-engine-core/assetmanagers/tac_mesh.h" // Mesh
#include "tac-rhi/render3/tac_render_api.h" // VertexDeclarations

namespace Tac
{
  struct ModelAssetManager
  {
    static void  Init();
    static void  Uninit();

    struct Params
    {
      AssetPathStringView        mPath;
      int                        mModelIndex{ 0 };
      Render::VertexDeclarations mOptVtxDecls;
    };

    //    the mesh will be loaded into the vertex format specified by vertex declarations.
    static Mesh* GetMesh( Params, Errors& );
  };

}
