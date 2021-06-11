#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacModelLoadSynchronous.h"
#include "src/common/assetmanagers/tacResidentModelFile.h"
#include "src/common/assetmanagers/tacModelAssetManagerBackend.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/math/tacMath.h"
#include "src/common/string/tacStringIdentifier.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacMemory.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/tacUtility.h"
#include "src/common/thirdparty/cgltf.h"

#include <map>

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
  //Mesh* ModelAssetManagerGetMesh( StringView path,
  //                                const Render::VertexDeclarations& vertexDeclarations,
  //                                Errors& errors )
  //{
  //  if( path.empty() )
  //    return nullptr;

  //  auto it = mMeshes.find( path );
  //  if( it != mMeshes.end() )
  //    return ( *it ).second;

  //  auto mesh = TAC_NEW Mesh;
  //  *mesh = LoadMeshSynchronous( path, vertexDeclarations, errors );
  //  mMeshes[ path ] = mesh;
  //  return mesh;
  //}


  static std::map< HashedValue, Mesh* > sTryNewTHingMeshes;

  static HashedValue HashAddVertexDeclaration( HashedValue hashedValue, const Render::VertexDeclaration& vertexDeclaration )
  {
    hashedValue = HashAddHash( hashedValue, ( HashedValue )vertexDeclaration.mAlignedByteOffset );
    hashedValue = HashAddHash( hashedValue, ( HashedValue )vertexDeclaration.mAttribute );
    hashedValue = HashAddHash( hashedValue, ( HashedValue )vertexDeclaration.mTextureFormat.mElementCount );
    hashedValue = HashAddHash( hashedValue, ( HashedValue )vertexDeclaration.mTextureFormat.mPerElementByteCount );
    hashedValue = HashAddHash( hashedValue, ( HashedValue )vertexDeclaration.mTextureFormat.mPerElementDataType );
    return hashedValue;
  }

  static HashedValue HashAddVertexDeclarations( HashedValue hashedValue, const Render::VertexDeclarations& vertexDeclarations )
  {
    for( const Render::VertexDeclaration vertexDeclaration : vertexDeclarations )
      hashedValue = HashAddVertexDeclaration( hashedValue, vertexDeclaration );
    return hashedValue;
  }


  Mesh* ModelAssetManagerGetMeshTryingNewThing( const char* path,
                                                const int iModel,
                                                const Render::VertexDeclarations& vertexDeclarations,
                                                Errors& errors )
  {
    if( !path || *path == '\0' )
      return nullptr;

    HashedValue hashedValue = HashAddString( path );
    hashedValue = HashAddVertexDeclarations( hashedValue, vertexDeclarations );
    hashedValue = HashAddHash( hashedValue, ( HashedValue )iModel );

    auto it = sTryNewTHingMeshes.find( hashedValue );
    if( it != sTryNewTHingMeshes.end() )
      return ( *it ).second;

    const char* pathExt = [ path ]()
    { 
      const int iDot = StringView( path ).find_last_of( "." );
      return iDot == StringView::npos
        ? ""
        : StringView( path ).substr( iDot + 1 );
    }( );
    MeshLoadFunction* meshLoadFunction = ModelLoadFunctionFind( pathExt );
    if( !meshLoadFunction )
      return nullptr;

    Mesh* mesh = TAC_NEW Mesh;
    *mesh = meshLoadFunction( path, iModel, vertexDeclarations, errors );
    TAC_ASSERT( errors.empty() );

    if( mesh )
      sTryNewTHingMeshes[ hashedValue ] = mesh;
    return mesh;
  }


} // namespace Tac
