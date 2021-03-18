#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacModelLoadSynchronous.h"
#include "src/common/graphics/tacRenderer.h"
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
  void ModelAssetManagerGetMesh( Mesh** mesh,
                                 StringView path,
                                 const VertexDeclarations& vertexDeclarations,
                                 Errors& errors )
  {
    auto it = mMeshes.find( path );
    if( it != mMeshes.end() )
    {
      *mesh = ( *it ).second;
      return;
    }

    *mesh = TAC_NEW Mesh;
    **mesh = LoadMeshSynchronous( path, vertexDeclarations, errors );
    mMeshes[ path ] = *mesh;
  }
}
