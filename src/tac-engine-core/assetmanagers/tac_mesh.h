#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix4.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"

#define TAC_HACK_COLOR_INTO_MESH() false

namespace Tac
{
  typedef Array< v3, 3 >            SubMeshTriangle;
  typedef Vector< SubMeshTriangle > SubMeshTriangles;

  struct MeshRay
  {
    v3 mPos;
    v3 mDir;
  };

  struct MeshRaycastResult
  {
    bool  mHit {};
    float mT   {};
  };

  struct SubMesh
  {
    MeshRaycastResult          SubMeshModelSpaceRaycast( MeshRay ) const;
    void                       ClearBuffers();

    Render::PrimitiveTopology  mPrimitiveTopology { Render::PrimitiveTopology::Unknown };

    Render::BufferHandle       mVertexBuffer      {};
    Render::BufferHandle       mIndexBuffer       {};

    SubMeshTriangles           mTris              {};
    int                        mIndexCount        {};
    int                        mVertexCount       {};
#if TAC_HACK_COLOR_INTO_MESH()
    v4                         mColor             { 1, 1, 1, 1 };
#endif
    String                     mName              {};
  };

  struct Mesh
  {
    MeshRaycastResult          MeshModelSpaceRaycast( MeshRay ) const;

    Vector< SubMesh >          mSubMeshes;
  };

} // namespace Tac
