#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  bool MeshRaycast::Result::IsValid() const { return mT; }

  auto MeshRaycast::Raycast( Ray ray ) const -> Result
  {
    int iClosest{};
    float tClosest{};

    const int triCount{ ( int )mTris.size() };
    for( int iTri{}; iTri < triCount; ++iTri )
    {
      if( const RayTriangle rayTri( ray, mTris[ iTri ] ); rayTri.mValid
          && ( !tClosest || rayTri.mT < tClosest ) )
      {
        tClosest = rayTri.mT;
        iClosest = iTri;
      }
    }

    if( !tClosest )
      return {};

    const Triangle closest{ mTris[ iClosest ] };
    const RayTriangle rayTri( ray, closest );
    return MeshRaycast::Result
    {
      .mT      { rayTri.mT },
      .mU      { rayTri.mU },
      .mV      { rayTri.mV },
      .mTriIdx { iClosest },
    };
  }

  // -----------------------------------------------------------------------------------------------

  void SubMesh::ClearBuffers()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyBuffer( mIndexBuffer );
    renderDevice->DestroyBuffer( mVertexBuffer );
    mIndexBuffer = {};
    mVertexBuffer = {};
  }

  // -----------------------------------------------------------------------------------------------

#if 0
  MeshRaycast::Result Mesh::MeshModelSpaceRaycast( MeshRaycast::Ray meshRay ) const
  {
    MeshRaycast::Result raycastResult{};

    for( const SubMesh& subMesh : mSubMeshes )
    {
      const MeshRaycast::Result submeshRaycastResult { subMesh.mMeshRaycast.Raycast( meshRay ) };
      if( !submeshRaycastResult.mHit )
        continue;

      if( raycastResult.mHit && submeshRaycastResult.mT > raycastResult.mT )
        continue;

      raycastResult = submeshRaycastResult;
    }

    return raycastResult;
  }
#endif

} // namespace Tac
