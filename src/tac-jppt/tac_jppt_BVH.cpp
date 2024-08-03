#include "tac_jppt_BVH.h"

namespace Tac::gpupt
{
  float AABB::Area() const
  {
    return {};
  }

  void AABB::Grow( const AABB& )
  {
  }

  void AABB::Grow( v3 )
  {
  }

  bool BVHNode::IsLeaf() const
  {
    return {};
  }

  BVH::BVH( Mesh* )
  {
  }
  void BVH::Build()
  {
  }
  void BVH::Refit()
  {
  }
  void BVH::Subdividie( u32 )
  {
  }
  void BVH::UpdateNodeBounds( u32 )
  {
  }
  float BVH::FindBestSplitPlane( BVHNode&, int&, float& )
  {
    return {};
  }
  float BVH::EvaluateSAH( BVHNode&, int&, float )
  {
    return {};
  }
  float BVH::CalculateNodeCost( BVHNode& )
  {
    return {};
  }

  Mesh::Mesh( const Shape& )
  {
  }

  void BVHInstance::SetTransform( const m4&, Vector< Mesh* >* )
  {
  }

  void SceneBVH::Destroy()
  {
  }

  SceneBVH* SceneBVH::CreateBVH( const Scene* )
  {
    return {};
  }

} // namespace Tac::gpupt

