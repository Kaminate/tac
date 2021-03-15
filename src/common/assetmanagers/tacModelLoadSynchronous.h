#pragma once

#include "src/common/containers/tacVector.h"
#include "src/common/tacString.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"

namespace Tac
{
#if 0

  struct MeshFileInfo
  {
    // mesh info, submesh info
    // material info
  };

  struct MeshLoadRequest
  {
    // If provided, the result will contain data from which a vertex buffer can be created from
    VertexDeclarations* mVertexDeclarations = nullptr;
  };

  struct MeshLoadResult
  {
    MeshFileInfo info;
    VertexBufferData;
  };
#endif


  //struct MeshLoadSynchronousResult
  //{
  //  struct Node
  //  {
  //    int              iMesh = -1;
  //    Vector< int >    mChildrenNodeIndexes;
  //  };
  //  // question 1: what if the entire mesh used one vertex buffer, and the submesh geometry
  //  //             simply has an offset and a count?
  //  // question 2: what if the material specifies the vertex format and not the caller
  //  struct SubMesh
  //  {
  //    int              mIMaterial = -1;
  //    int              mIGeometry = -1;
  //  };
  //  struct Geometry
  //  {
  //  };
  //  struct Mesh
  //  {
  //    Vector< SubMesh > mSubMeshes;
  //  };
  //  // Vector< Node > mNodes;
  //  //struct Node
  //  //{
  //  //  int iChild = 0;
  //  //  int iNext = 0;
  //  //} mNodes[ 100 ];
  //  //Node* Root() { return mNodes; }
  //  struct Material
  //  {
  //    v3               mColor;
  //    String           mPath;
  //  };
  //  //Node*              mRoot = nullptr;
  //  Vector< Geometry > mGeometry;
  //  Vector< Node >     mNodes;
  //  Vector< Material > mMaterials;
  //  Vector< Mesh >     mMeshes;
  //  //Mesh*              mMeshes;
  //  //int                mMeshCount;
  //};


  struct Mesh;
  struct StringView;
  struct VertexDeclarations;
  struct Errors;
  Mesh LoadMeshSynchronous( const StringView& path,
                            const VertexDeclarations&,
                            Errors& );

  
  //struct IEditorLoadInfo
  //{
  //  struct MeshLoadInfo
  //  {

  //  };

  //  struct Node
  //  {
  //    MeshLoadInfo* n;
  //    m4 mLocalTransformation;
  //  };

  //  void foo()
  //  {

  //    if( "Instantiate Prefab" ) // button
  //    {

  //    }
  //  }

  //  struct World;
  //  struct Entity;
  //  World* editWorld;
  //  Entity* prefabEntity;
  //};


  //IEditorLoadInfo* LoadEditorModelFileInfo( const StringView&, Errors& );

  //struct GameRequestedLoadInfoComponentShit
  //{
  //  struct material
  //  {
  //    StringID texturepath_diffuse;
  //    StringID texturepath_specular;
  //  };

  //  struct submesh
  //  {
  //    material m;

  //    StringID meshgeoname;
  //  };

  //  struct mesh 
  //  {
  //    submesh* submeshes;
  //    int submeshcount;
  //  };

  //  StringID path;
  //  int iMesh;
  //};
}

