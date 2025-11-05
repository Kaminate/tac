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
    struct Result
    {
      bool  mHit {};
      float mT   {};
    };

    using SubMeshTriangles = Vector< Triangle >;

    Result Raycast( Ray ) const;
    static Result RaycastTri( Ray, const Triangle& );

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

  // also used for tac_radiosity_bake_presentation.cpp
  struct JPPTCPUMeshData
  {
    // data for jppt
    // not including vtx colors because they are
    // 1) hardcoded into https://github.com/jacquespillet/gpupt_blog/blob/Part_2/src/Scene.cpp
    // 2) removed in     https://github.com/jacquespillet/gpupt_blog/blob/Part_3/src/Scene.cpp
    //    (they are moved into TLASInstancesBuffer[InstanceIndex].MaterialIndex;

    // Topology assumed to be triangle list
    using IndexType = int;
    Vector< v3 >        mPositions;
    Vector< v2 >        mTexCoords;
    Vector< v3 >        mNormals;
    Vector< v3 >        mTangents;
    Vector< v3 >        mBitangents;
    Vector< IndexType > mIndexes;
  };

  struct Mesh
  {
    Vector< SubMesh >               mSubMeshes             {};
    Render::VertexDeclarations      mVertexDecls           {};
    Render::BufferHandle            mGPUInputLayoutBuffer  {};
    Render::IBindlessArray::Binding mGPUInputLayoutBinding {};
    MeshRaycast                     mMeshRaycast           {};
    JPPTCPUMeshData                 mJPPTCPUMeshData       {};
  };

} // namespace Tac
