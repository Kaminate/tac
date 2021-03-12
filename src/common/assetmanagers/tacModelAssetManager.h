#pragma once
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/containers/tacArray.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacVector4.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/graphics/tacRenderer.h"
#include <map>

namespace Tac
{
  typedef Array< v3, 3 >            SubMeshTriangle;
  typedef Vector< SubMeshTriangle > SubMeshTriangles;

  v3 GetNormal( const SubMeshTriangle& );

  struct SubMesh
  {
    void                       SubMeshModelSpaceRaycast( v3 inRayPos,
                                                         v3 inRayDir,
                                                         bool* outHit,
                                                         float* outDist );
    Render::VertexBufferHandle mVertexBuffer;
    Render::IndexBufferHandle  mIndexBuffer;
    SubMeshTriangles           mTris;
    int                        mIndexCount = 0;
  };

  struct Mesh
  {
    void                       MeshModelSpaceRaycast( v3 inRayPos,
                                                      v3 inRayDir,
                                                      bool* outHit,
                                                      float* outDist );
    Vector< SubMesh >          mSubMeshes;
    m4                         mTransform = m4::Identity();
    m4                         mTransformInv = m4::Identity();
  };

  void                       ModelAssetManagerUninit();

  //                         the mesh will be loaded into the vertex format specified by vertex declarations.
  void                       ModelAssetManagerGetMesh( Mesh** mesh,
                                                       StringView path,
                                                       const VertexDeclarations&,
                                                       Errors& );
}
