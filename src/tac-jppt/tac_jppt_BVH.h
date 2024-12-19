#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-rhi/render3/tac_render_api.h"

#include "tac_jppt_Scene.h"

namespace Tac::gpupt
{

#define static_assert_gpu_padded( S ) static_assert( sizeof( S ) % 16 == 0 )

  const float inf { 1e30f };

  struct AABB32
  {
    float Area() const;
    void  Grow( const AABB32& );
    void  Grow( v3 );

    v3    mMin{ inf };
    float mPad0;
    v3    mMax{ -inf };
    float mPad1;
  };

  static_assert_gpu_padded( AABB32 );

  //struct Ray
  //{
  //  v3 mOrigin;
  //  v3 mDirection;
  //  v3 mInvDir;
  //};

  //struct RayPayload
  //{
  //  float mDirection;
  //  float mU;
  //  float mV;
  //  v3    mEmission; // ???
  //  u32   mInstanceIndex; // ???
  //  u32   mMaterialIndex; // ???
  //  u32   mPrimitiveIndex; // ???
  //  u32   mRandomState; // ???
  //  u8    mDepth; // ???
  //};

  struct Bin
  {
      AABB32 mBounds        {};
      u32    mTriangleCount {};
  };

  struct BVHTriangle
  {
    v3    mV0       {};
    float mPad0     {};
    v3    mV1       {};
    float mPad1     {};
    v3    mV2       {};
    float mPad2     {};
    v3    mCentroid {};
    float mPad3     {};
  };

  static_assert_gpu_padded( BVHTriangle );

  struct VertexExtraData
  {
    v3    mNormal  {};
    float mPad0    {};
    v2    mUV      {};
    v2    mPad1    {};
    v4    mColor   {};
    v4    mTangent {};
  };

  struct BVHTriangleExtraData
  {
    VertexExtraData mVertexExtraDatas[ 3 ] {};
  };

  static_assert_gpu_padded( BVHTriangleExtraData );

  struct BVHNode
  {
    bool IsLeaf() const;

    AABB32 mAABB;
    union
    {
      u32  mLeftChild;
      u32  mFirstTriangleIndex; // this is actually a index index
    };
    u32  mTriangleCount;
    v2   mPad;
  };

  static_assert_gpu_padded( BVHNode );

  struct BVH;
  struct BVHMesh;

  struct BVH
  {
    void  Build();
    void  Refit();
    void  Subdivide( u32 );
    void  UpdateNodeBounds( u32 );
    float FindBestSplitPlane( const BVHNode&, int&, float& );
    float EvaluateSAH( const BVHNode&, int, float );
    float CalculateNodeCost( const BVHNode& );

    BVHMesh*          mMesh            {};
    Vector< u32 >     mTriangleIndices {};
    Vector< BVHNode > mBVHNodes        {};
    u32               mNodesUsed       {};
  };

  struct BVHMesh
  {
    void SetShape( const Shape& );

    BVH                            mBVH{};
    Vector< BVHTriangle >          mTriangles;
    Vector< BVHTriangleExtraData > mTrianglesExtraData;
  };

  struct TLASNode
  {
    bool IsLeaf() const { return !mLeftRight; }

    v3  mAABBMin;

    // this is (left + right << 16)
    // could maybe instead use u16 left, u16 right?
    // https://github.com/microsoft/DirectXShaderCompiler/wiki/16-Bit-Scalar-Types
    //   supported since Shader Model 6.2  
    u32 mLeftRight;
    v3  mAABBMax;

    // an index into SceneBVH::mTLASInstancesBuffer, which is basically
    // Vector<BVHInstance> SceneBVH::mInstances
    u32 mBLAS;
  };

  static_assert_gpu_padded( TLASNode );

  // this is referenced as
  // Span< BVHInstance > TLAS::mBLAS, and
  // Vector< BVHInstance > SceneBVH::mInstances
  struct BVHInstance
  {
    // why sends a vector*? why not const?
    void SetTransform( const m4& transform,
                       const m4& transformInv,
                       AABB32 );

    m4     mInverseTransform;
    m4     mTransform;
    m4     mNormalTransform; // ???
    AABB32 mBounds;
    u32    mMeshIndex; // ??? used by the shader, indexes into SceneBVH::mIndexDataBuffer 
    u32    mIndex{}; // ??? index into SceneBVH::mInstances;
    v2     mPad;
  };

  static_assert_gpu_padded( BVHInstance );

  struct TLAS
  {
    void Build();

  private:
    int FindBestMatch( int* List, int N, int A );

  public:

    Span< BVHInstance >    mBLAS;
    Vector< TLASNode >     mNodes;
    u32                    mNodesUsed{};
  };

  // ???
  struct BVHIndexData
  {
    u32 mTriangleTriangleDataStartInx;
    u32 mIndicesDataStartInx;
    u32 mBVHNodeDataStartInx;
    u32 mTriangleCount;
  };

  struct SceneBVH
  {
    void CreateBuffers( Errors& );

    static SceneBVH*            CreateBVH( const Scene*,Errors& );
    static Render::BufferHandle CreateBuffer( int, const void*, int, const char*, Errors&);

    TLAS                           mTLAS;
    Vector< BVHMesh >              mMeshes;
    Vector< BVHInstance >          mInstances;
    Vector< BVHIndexData >         mIndexData;
    Vector< BVHTriangle >          mAllTriangles;
    Vector< BVHTriangleExtraData > mAllTrianglesEx;
    Vector< u32 >                  mAllTriangleIndices;
    Vector< BVHNode >              mAllBVHNodes;

    Render::BufferHandle mIndexDataBuffer;
    Render::BufferHandle mAllTrianglesBuffer;
    Render::BufferHandle mAllTrianglesExBuffer;
    Render::BufferHandle mAllTriangleIndicesBuffer;
    Render::BufferHandle mAllBVHNodesBuffer;
    Render::BufferHandle mTLASInstancesBuffer;
    Render::BufferHandle mTLASNodeBuffer;

    const Scene*                mScene{};
  };

} // namespace Tac::gpupt

