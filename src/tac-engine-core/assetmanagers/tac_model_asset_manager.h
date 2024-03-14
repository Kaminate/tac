#pragma once

namespace Tac::Render { struct VertexDeclarations; }
namespace Tac { struct Errors; struct Mesh; struct AssetPathStringView; }

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
