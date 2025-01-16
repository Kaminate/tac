#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-rhi/render3/tac_render_api.h"

//#include "tac_jppt_Scene.h"

namespace Tac
{

#define static_assert_gpu_padded( S ) static_assert( sizeof( S ) % 16 == 0 )

  const float inf { 1e30f };

  struct AABB32
  {
    float Area() const;
    void  Grow( const AABB32& );
    void  Grow( v3 );

    v3    mMin{ inf };
    TAC_PAD_BYTES( 4 );
    v3    mMax{ -inf };
    TAC_PAD_BYTES( 4 );
  };

  static_assert( sizeof( AABB32 ) == 32 );
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
    TAC_PAD_BYTES( 4 );
    v3    mV1       {};
    TAC_PAD_BYTES( 4 );
    v3    mV2       {};
    TAC_PAD_BYTES( 4 );
    v3    mCentroid {};
    TAC_PAD_BYTES( 4 );
  };

  static_assert_gpu_padded( BVHTriangle );

  struct VertexExtraData
  {
    v3    mNormal  {};
    TAC_PAD_BYTES( 4 );
    v2    mUV      {};
    TAC_PAD_BYTES( 8 );
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
      u32  mLeftChild; // index into BVH::mBVHNodes
      u32  mFirstTriangleIndex; // this is actually a index into BVH::mTriangleIndices
    };
    u32  mTriangleCount;
    TAC_PAD_BYTES( 8 );
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
    void SetShape( const Model* );
    void DebugImguiBVHMesh(Debug3DDrawData* ) const;
    void DebugImguiBVHNode(Debug3DDrawData*, const BVHNode&) const;

    BVH                            mBVH                {};
    Vector< BVHTriangle >          mTriangles          {};
    Vector< BVHTriangleExtraData > mTrianglesExtraData {};
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

    m4     mInverseTransform {};
    m4     mTransform        {};
    m4     mNormalTransform  {}; // ???
    AABB32 mBounds           {};
    u32    mMeshIndex        {}; // ??? used by the shader, indexes into SceneBVH::mIndexDataBuffer 
    u32    mIndex            {}; // ??? index into SceneBVH::mInstances;
    TAC_PAD_BYTES( 8 )       {};
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

    static SceneBVH*            CreateBVH( const World*, Errors& );
    static Render::BufferHandle CreateBuffer( int, const void*, int, const char*, Errors& );

    void DebugImguiSceneBVH( Debug3DDrawData* ) const;


    // ----------------------------------------------------------------------------------------
    void CreateAllTrianglesBuffer( Errors& );
    void CreateAllTrianglesExBuffer( Errors& );
    void CreateAllBVHNodesBuffer( Errors& );
    void CreateAllTriangleIndicesBuffer( Errors& );
    void CreateIndexDataBuffer( Errors& );
    void CreateTLASInstancesBuffer( Errors& );
    void CreateTLASNodeBuffer( Errors& );
    // --------------------------------------------------------------------------





    TLAS                           mTLAS;
    Vector< BVHMesh >              mMeshes;
    Vector< BVHInstance >          mInstances;
    Vector< BVHIndexData >         mIndexData;          // gpu --> mIndexDataBuffer
    Vector< BVHTriangle >          mAllTriangles;       // gpu --> mAllTrianglesBuffer
    Vector< BVHTriangleExtraData > mAllTrianglesEx;     // gpu --> mAllTrianglesExBuffer
    Vector< u32 >                  mAllTriangleIndices; // gpu --> mAllTriangleIndicesBuffer
    Vector< BVHNode >              mAllBVHNodes;        // gpu --> mAllBVHNodesBuffer

    Render::BufferHandle mIndexDataBuffer;           // cpu --> mIndexData
    Render::BufferHandle mAllTrianglesBuffer;        // cpu --> mAllTriangles
    Render::BufferHandle mAllTrianglesExBuffer;      // cpu --> mAllTrianglesEx
    Render::BufferHandle mAllTriangleIndicesBuffer;  // cpu --> mAllTriangleIndices
    Render::BufferHandle mAllBVHNodesBuffer;         // cpu --> mAllBVHNodes
    Render::BufferHandle mTLASInstancesBuffer;       // cpu --> mTLAS.mBLAS / mInstances?
    Render::BufferHandle mTLASNodeBuffer;            // cpu --> mTLAS.mNodes
  };

} // namespace Tac

