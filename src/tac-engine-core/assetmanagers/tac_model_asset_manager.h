#pragma once

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-std-lib/tac_core.h"

namespace Tac
{

  void  ModelAssetManagerInit();
  void  ModelAssetManagerUninit();

  //    the mesh will be loaded into the vertex format specified by vertex declarations.
  Mesh* ModelAssetManagerGetMeshTryingNewThing( const AssetPathStringView&,
                                                int iModel,
                                                const Render::VertexDeclarations&,
                                                Errors& );

}
