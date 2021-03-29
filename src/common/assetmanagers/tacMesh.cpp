#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/math/tacVector4.h"

namespace Tac
{
  static bool RaycastTriangle( const v3& p0,
                               const v3& p1,
                               const v3& p2,
                               const v3& rayPos,
                               const v3& normalizedRayDir,
                               float & dist )
  {
    v3 edge2 = p2 - p0;
    v3 edge1 = p1 - p0;
    v3 b = rayPos - p0;
    v3 p = Cross( normalizedRayDir, edge2 );
    v3 q = Cross( b, edge1 );
    float pdotv1 = Dot( p, edge1 );
    float t = Dot( q, edge2 ) / pdotv1;
    float u = Dot( p, b ) / pdotv1;
    float v = Dot( q, normalizedRayDir ) / pdotv1;
    if( t > 0 && u >= 0 && v >= 0 && u + v <= 1 )
    {
      dist = t;
      return true;
    }
    return false;
  }

  void SubMesh::SubMeshModelSpaceRaycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist )
  {
    bool submeshHit = false;
    float submeshDist = 0;
    int triCount = ( int )mTris.size();
    for( int iTri = 0; iTri < triCount; ++iTri )
    {
      const SubMeshTriangle& tri = mTris[ iTri ];
      float triDist;
      const bool triHit = RaycastTriangle( tri[ 0 ],
                                           tri[ 1 ],
                                           tri[ 2 ],
                                           inRayPos,
                                           inRayDir,
                                           triDist );
      if( !triHit )
        continue;
      if( submeshHit && triDist > submeshDist )
        continue;
      submeshDist = triDist;
      submeshHit = true;
    }
    *outHit = submeshHit;
    *outDist = submeshDist;
  }

  void Mesh::MeshModelSpaceRaycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist )
  {
    v4 inRayPos4 = { inRayPos, 1 };
    inRayPos4 = mTransformInv * inRayPos4;
    inRayPos = inRayPos4.xyz();

    bool meshHit = false;
    float meshDist = 0;
    for( SubMesh& subMesh : mSubMeshes )
    {
      bool subMeshHit = false;
      float submeshDist = 0;
      subMesh.SubMeshModelSpaceRaycast( inRayPos, inRayDir, &subMeshHit, &submeshDist );
      if( !subMeshHit )
        continue;
      if( meshHit && submeshDist > meshDist )
        continue;
      meshDist = submeshDist;
      meshHit = true;
    }
    *outHit = meshHit;
    *outDist = meshDist;
  }

}