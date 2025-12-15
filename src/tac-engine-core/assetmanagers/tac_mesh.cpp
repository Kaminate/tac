#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{


  // -----------------------------------------------------------------------------------------------

  bool MeshRaycast::Result::IsValid() const { return mT > 0; }

  auto MeshRaycast::ConvertWorldToModelRay( const Ray& ray_worldspace, const m4& model_to_world ) -> Ray
  {
    bool invExists;
    const m4 world_to_model{ m4::Inverse( model_to_world, &invExists ) };
    TAC_ASSERT( invExists );
    return Ray{
        .mOrigin    { ( world_to_model * v4( ray_worldspace.mOrigin, 1 ) ).xyz() },
        .mDirection { ( world_to_model * v4( ray_worldspace.mDirection, 0 ) ).xyz() },
    };
  }

  auto MeshRaycast::Raycast_modelspace( const Ray& ray_modelspace ) const -> Result
  {
    int iClosest{};
    float tClosest{};

    const int triCount{ ( int )mTris.size() };
    for( int iTri{}; iTri < triCount; ++iTri )
    {
      if( const RayTriangle rayTri( ray_modelspace, mTris[ iTri ] ); rayTri.mValid
          && ( !tClosest || rayTri.mT < tClosest ) )
      {
        tClosest = rayTri.mT;
        iClosest = iTri;
      }
    }

    if( !tClosest )
      return {};

    const Triangle closest{ mTris[ iClosest ] };
    const RayTriangle rayTri( ray_modelspace, closest );
    return MeshRaycast::Result
    {
      .mT      { rayTri.mT },
      .mU      { rayTri.mU },
      .mV      { rayTri.mV },
      .mTriIdx { iClosest },
    };
  }

  auto MeshRaycast::Raycast_worldspace( const Ray& ray_worldspace, const m4& model_to_world ) const -> Result
  {
    const Ray ray_modelspace{ ConvertWorldToModelRay( ray_worldspace, model_to_world ) };
    const MeshRaycast::Result result_modelspace{ Raycast_modelspace( ray_modelspace ) };
    if( !result_modelspace.IsValid() )
      return {};
    const v3 hitPoint_modelspace{ray_modelspace.mOrigin + result_modelspace.mT * ray_modelspace.mDirection };
    const v3 hitPoint_worldspace{ ( model_to_world * v4( hitPoint_modelspace, 1 ) ).xyz() };
    dynmc MeshRaycast::Result result_worldspace{ result_modelspace };
    result_worldspace.mT = Dot( hitPoint_worldspace - ray_worldspace.mOrigin, ray_worldspace.mDirection )
      / ray_worldspace.mDirection.Length();
    return result_worldspace;
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


} // namespace Tac
