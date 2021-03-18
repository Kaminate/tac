#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacModelLoadSynchronous.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/string/tacStringIdentifier.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacMemory.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacUtility.h"
#include "src/common/thirdparty/cgltf.h"

namespace Tac
{
  static std::map< StringID, Mesh* > mMeshes;

  void ModelAssetManagerUninit()
  {
    for( auto pair : mMeshes )
    {
      Mesh* mesh = pair.second;
      for( SubMesh& submesh : mesh->mSubMeshes )
      {
        Render::DestroyIndexBuffer( submesh.mIndexBuffer, TAC_STACK_FRAME );
        Render::DestroyVertexBuffer( submesh.mVertexBuffer, TAC_STACK_FRAME );
      }
    }
  }

  // todo: multithreading
  Mesh* ModelAssetManagerGetMesh( StringView path,
                                  const Render::VertexDeclarations& vertexDeclarations,
                                  Errors& errors )
  {
    auto it = mMeshes.find( path );
    if( it != mMeshes.end() )
      return ( *it ).second;

    auto mesh = TAC_NEW Mesh;
    *mesh = LoadMeshSynchronous( path, vertexDeclarations, errors );
    mMeshes[ path ] = mesh;
    return mesh;
  }

  Mesh* ModelAssetManagerGetMeshTryingNewThing( const char* path,
                                                int iModel,
                                                const Render::VertexDeclarations& vertexDeclarations,
                                                Errors& errors )
  {
    TAC_UNUSED_PARAMETER(path);
    TAC_UNUSED_PARAMETER(iModel);
    TAC_UNUSED_PARAMETER(vertexDeclarations);
    TAC_UNUSED_PARAMETER(errors);



    TAC_CRITICAL_ERROR_UNIMPLEMENTED;
    return nullptr;
  }
}
