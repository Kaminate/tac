#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  MeshRaycast::Result MeshRaycast::RaycastTri( Ray meshRay, const SubMeshTriangle& tri )
  {
    const RayTriangle::Ray rayTriangleRay
    {
      .mOrigin{ meshRay.mPos },
      .mDirection{ meshRay.mDir },
    };

    const RayTriangle::Triangle rayTriangleTriangle
    {
      .mP0{ tri[ 0 ] },
      .mP1{ tri[ 1 ] },
      .mP2{ tri[ 2 ] },
    };

    const RayTriangle::Output rayTriangleOutput{
      RayTriangle::Solve( rayTriangleRay, rayTriangleTriangle ) };

    return Result{ .mHit{ rayTriangleOutput.mValid }, .mT{ rayTriangleOutput.mT } };
  }

  MeshRaycast::Result MeshRaycast::Raycast( Ray meshRay ) const
  {
    MeshRaycast::Result meshResult;

    const int triCount{ ( int )mTris.size() };
    for( int iTri{}; iTri < triCount; ++iTri )
      if( const MeshRaycast::Result triResult{ RaycastTri( meshRay, mTris[ iTri ] ) };
          triResult.mHit && ( !meshResult.mT || triResult.mT < meshResult.mT ) )
        meshResult = triResult;

    return meshResult;
  }


  void              SubMesh::ClearBuffers()
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
