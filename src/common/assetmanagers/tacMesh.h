#pragma once

#include "src/common/string/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/containers/tacArray.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  typedef Array< v3, 3 >            SubMeshTriangle;
  typedef Vector< SubMeshTriangle > SubMeshTriangles;

  struct SubMesh
  {
    void                       SubMeshModelSpaceRaycast( v3 inRayPos,
                                                         v3 inRayDir,
                                                         bool* outHit,
                                                         float* outDist ) const;
    Render::VertexBufferHandle mVertexBuffer;
    Render::IndexBufferHandle  mIndexBuffer;
    SubMeshTriangles           mTris;
    int                        mIndexCount = 0;
    String                     mName;
  };

  struct Mesh
  {
    void                       MeshModelSpaceRaycast( v3 inRayPos,
                                                      v3 inRayDir,
                                                      bool* outHit,
                                                      float* outDist ) const;
    Vector< SubMesh >          mSubMeshes;
    m4                         mTransform = m4::Identity();
    m4                         mTransformInv = m4::Identity();
  };

}
