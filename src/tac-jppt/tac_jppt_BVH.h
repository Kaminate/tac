#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-rhi/render3/tac_render_api.h"

#include "tac_jppt_Scene.h"

namespace Tac::gpupt
{

#define static_assert_gpu_padded( S ) static_assert( sizeof( S ) % 16 == 0 )

  const float inf { 1e30f };

  // I want to name this like, padded aabb
  struct AABB
  {
    float Area() const;
    void  Grow( const AABB& );
    void  Grow( v3 );

    v3    mMin{ inf };
    float mPad0;
    v3    mMax{ -inf };
    float mPad1;
  };

  static_assert_gpu_padded( AABB );

  struct Ray
  {
    v3 mOrigin;
    v3 mDirection;
    v3 mInvDir;
  };

  struct RayPayload
  {
    float mDirectoin;
    float mU;
    float mV;
    v3    mEmission; // ???
    u32   mInstanceIndex; // ???
    u32   mMaterialIndex; // ???
    u32   mPrimitiveIndex; // ???
    u32   mRandomState; // ???
    u8    mDepth; // ???
  };

  struct Triangle
  {
    v3    mV0;
    float mPad0;

    v3    mV1;
    float mPad1;

    v3    mV2;
    float mPad2;

    v3    mCentroid;
    float mPad3;
  };

  static_assert_gpu_padded( Triangle );

  struct TriangleExtraData
  {
    v3    mNormal0;
    float mPad0;

    v3    mNormal1;
    float mPad1;

    v3    mNormal2;
    float mPad2;

    v2    mUV0;
    v2    mUV1;
    v2    mUV2;
    v2    mPad3;

    v4    mColor0;
    v4    mColor1;
    v4    mColor2;

    v4    mTangent0;
    v4    mTangent1;
    v4    mTangent2;
  };

  static_assert_gpu_padded( TriangleExtraData );

  struct BVHNode
  {
    bool IsLeaf() const;

    AABB mAABB;
    u32  mLeftChildOrFirst;
    u32  mTriangleCount;
    v2   mPad;
  };

  static_assert_gpu_padded( BVHNode );

  struct BVH;
  struct Mesh;

  struct BVH
  {
    BVH( Mesh* );
    void  Build();
    void  Refit();
    void  Subdividie( u32 );
    void  UpdateNodeBounds( u32 );
    float FindBestSplitPlane( BVHNode&, int&, float& );
    float EvaluateSAH( BVHNode&, int&, float );
    float CalculateNodeCost( BVHNode& );

    Mesh*             mMesh{};
    Vector< u32 >     mTriangleIndices;
    Vector< BVHNode > mBVHNodes;
    u32               mNodesUsed{ 1 };
    u32               mRootNodeIndex{};
  };


  struct Vertex
  {
    v3 mPosition;
    v3 mNormal;
    v3 mTangent;
    v3 mMatInx; // ???
  };

  struct Mesh
  {
    Mesh( const Shape& );

    BVH*                        mBVH{};
    Vector< Triangle >          mTriangles;
    Vector< TriangleExtraData > mTrianglesExtraData;
  };

  struct TLASNode
  {
    bool IsLeaf() const { return !mLeftRight; }

    v3  mAABBMin;
    u32 mLeftRight;
    v3  mAABBMax;
    u32 mBLAS;
  };

  static_assert_gpu_padded( TLASNode );

  struct BVHInstance
  {
    // why sends a vector*? why not const?
    void SetTransform( const m4&, Vector< Mesh* >* );

    m4   mInverseTransform;
    m4   mTransform;
    m4   mNormalTransform; // ???
    AABB mBounds;
    u32  mMeshIndex;
    u32  mIndex{};
    v2   mPad;
  };

  static_assert_gpu_padded( BVHInstance );

  struct TLAS
  {
    Vector< BVHInstance >* mBLAS; // why a pointer?
    Vector< TLASNode >     mNodes;
    u32                    mNodesUsed{};
  };

  // ???
  struct IndexData
  {
    u32 mTriangleTriangleDataStartInx;
    u32 mIndicesDataStartInx;
    u32 mBVHNodeDataStartInx;
    u32 mTriangleCount;
  };

  struct SceneBVH
  {
    void Destroy();
    static SceneBVH* CreateBVH( const Scene* );

    TLAS                        mTLAS;
    Vector< Mesh* >             mMeshes;
    Vector< BVHInstance >       mInstances;
    Vector< IndexData >         mIndexData;
    Vector< Triangle >          mAllTriangles;
    Vector< TriangleExtraData > mAllTrianglesEx;
    Vector< u32 >               mAllTriangleIndices;
    Vector< BVHNode >           mAllBVHNodes;

    Render::BufferHandle        mTrianglesBuffer;
    Render::BufferHandle        mTrianglesExBuffer;
    Render::BufferHandle        mBVHBuffer;
    Render::BufferHandle        mIndicesBuffer;
    Render::BufferHandle        mIndexDataBuffer;
    Render::BufferHandle        mTLASInstancesBuffer;
    Render::BufferHandle        mTLASNodeBuffer;

    Scene*                      mScene{};
  };

} // namespace Tac::gpupt

