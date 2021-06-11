#pragma once

#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct Mesh;
  struct StringView;
  struct Errors;

  void  ModelAssetManagerUninit();

  //    the mesh will be loaded into the vertex format specified by vertex declarations.
  Mesh* ModelAssetManagerGetMeshTryingNewThing( const char* path,
                                                int iModel,
                                                const Render::VertexDeclarations&,
                                                Errors& );

}
