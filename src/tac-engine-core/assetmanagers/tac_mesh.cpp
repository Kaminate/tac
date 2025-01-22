#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{
  static bool RaycastTriangle( const v3& p0,
                               const v3& p1,
                               const v3& p2,
                               const v3& rayPos,
                               const v3& normalizedRayDir,
                               float & dist )
  {
    v3 edge2 { p2 - p0 };
    v3 edge1{ p1 - p0 };
    v3 b { rayPos - p0 };
    v3 p { Cross( normalizedRayDir, edge2 ) };
    v3 q { Cross( b, edge1 ) };
    float pdotv1 { Dot( p, edge1 ) };
    float t { Dot( q, edge2 ) / pdotv1 };
    float u { Dot( p, b ) / pdotv1 };
    float v { Dot( q, normalizedRayDir ) / pdotv1 };
    if( t > 0 && u >= 0 && v >= 0 && u + v <= 1 )
    {
      dist = t;
      return true;
    }
    return false;
  }

  // -----------------------------------------------------------------------------------------------

  MeshRaycast::Result MeshRaycast::RaycastTri( Ray meshRay, const SubMeshTriangle& tri )
  {
    float triDist{};
    const bool triHit{ RaycastTriangle( tri[ 0 ],
                                        tri[ 1 ],
                                        tri[ 2 ],
                                        meshRay.mPos,
                                        meshRay.mDir,
                                        triDist ) };

    return Result{ .mHit { triHit }, .mT{ triDist } };
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
