#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix4.h"
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"

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

    Render::PrimitiveTopology  mPrimitiveTopology = Render::PrimitiveTopology::Unknown;
    Render::BufferHandle mVertexBuffer;
    Render::BufferHandle mIndexBuffer;
    SubMeshTriangles           mTris;
    int                        mIndexCount { 0 };
    int                        mVertexCount { 0 };
    String                     mName;
  };

  struct Mesh
  {
    void                       MeshModelSpaceRaycast( v3 inRayPos,
                                                      v3 inRayDir,
                                                      bool* outHit,
                                                      float* outDist ) const;
    Vector< SubMesh >          mSubMeshes;
  };

} // namespace Tac
