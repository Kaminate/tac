#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_array.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/dataprocess/tac_hash.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/assetmanagers/tac_gpu_input_layout.h"

namespace Tac
{
  struct MeshRaycast
  {
    struct Ray
    {
      v3 mPos {};
      v3 mDir {};
    };

    struct Result
    {
      bool  mHit {};
      float mT   {};
    };

    using SubMeshTriangle  = Array< v3, 3 >;
    using SubMeshTriangles = Vector< SubMeshTriangle >;

    Result Raycast( Ray ) const;

    SubMeshTriangles mTris {};
  };


  struct SubMesh
  {
    void                            ClearBuffers();

    Render::PrimitiveTopology       mPrimitiveTopology   {};
    Render::BufferHandle            mVertexBuffer        {};
    Render::BufferHandle            mIndexBuffer         {};
    Render::IBindlessArray::Binding mVertexBufferBinding {};
    int                             mIndexCount          {};
    int                             mVertexCount         {};
    String                          mName                {};
  };

  struct Mesh
  {
    Vector< SubMesh >               mSubMeshes             {};
    Render::VertexDeclarations      mVertexDecls           {};
    Render::BufferHandle            mGPUInputLayoutBuffer  {};
    Render::IBindlessArray::Binding mGPUInputLayoutBinding {};
    MeshRaycast                     mMeshRaycast           {};
  };

} // namespace Tac
