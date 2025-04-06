#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/graphics/material/tac_material.h"
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
    v3    Center() const { return ( mMin + mMax ) / 2; }

    v3    mMin         { inf };
    TAC_PAD_BYTES( 4 ) {};
    v3    mMax         { -inf };
    TAC_PAD_BYTES( 4 ) {};
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
    v3    mV0          {};
    TAC_PAD_BYTES( 4 ) {};
    v3    mV1          {};
    TAC_PAD_BYTES( 4 ) {};
    v3    mV2          {};
    TAC_PAD_BYTES( 4 ) {};
    v3    mCentroid    {};
    TAC_PAD_BYTES( 4 ) {};

    struct Index32{ u32 mIndex{}; };
  };

  static_assert_gpu_padded( BVHTriangle );

  struct BVHTriangles : public Vector< BVHTriangle >
  {
    dynmc BVHTriangle& GetTriangle( BVHTriangle::Index32 i ) dynmc { return (*this)[ i.mIndex ]; }
    const BVHTriangle& GetTriangle( BVHTriangle::Index32 i ) const { return (*this)[ i.mIndex ]; }
  };

  struct VertexExtraData
  {
    v3    mNormal      {};
    TAC_PAD_BYTES( 4 ) {};
    v2    mUV          {};
    TAC_PAD_BYTES( 8 ) {};
    v4    mColor       {};
    v4    mTangent     {};
  };

  struct BVHTriangleExtraData
  {
    VertexExtraData mVertexExtraDatas[ 3 ] {};
  };

  struct BVHTriangleExtraDatas : Vector< BVHTriangleExtraData >
  {
    dynmc BVHTriangleExtraData& GetTriangleExtraData( BVHTriangle::Index32 i ) dynmc { return (*this)[ i.mIndex ]; }
    const BVHTriangleExtraData& GetTriangleExtraData( BVHTriangle::Index32 i ) const { return (*this)[ i.mIndex ]; }
  };

  static_assert_gpu_padded( BVHTriangleExtraData );

  struct BVHNode
  {
    bool IsLeaf() const;
    u32 GetLChild() const;
    u32 GetRChild() const;

    AABB32 mAABB; // modelspace?
    union
    {
      u32  mLeftChild; // index into BVH::mBVHNodes
      u32  mFirstTriangleIndex{}; // this is actually a index into BVH::mTriangleIndices
    };
    u32  mTriangleCount{};
    u32  mDepth{}; 
    u32  mParent{}; 
  };


  static_assert( sizeof( BVHNode ) == 48 );
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
    Vector< u32 >     mTriangleIndices {}; // Indexes into BVHMesh::mTriangles
    Vector< BVHNode > mBVHNodes        {};
    u32               mNodesUsed       {};
    u32               mMaxDepth        {};
  };

  struct BVHMesh
  {
    void SetShape( const Model* );

    BVH                            mBVH                {};
    BVHTriangles                   mTriangles          {};
    BVHTriangleExtraDatas          mTrianglesExtraData {};
  };

  struct BVHMeshes : public Vector< BVHMesh >
  {
    struct Index32 { u32 mIndex{}; };

    dynmc BVHMesh& GetMesh( Index32 i ) dynmc { return ( *this )[ i.mIndex ]; }
    const BVHMesh& GetMesh( Index32 i ) const { return ( *this )[ i.mIndex ]; }
  };

  struct TLASNode
  {
    bool IsLeaf() const;
    u32 GetLChild() const;
    u32 GetRChild() const;
    AABB32 GetAABB() const;

    v3  mAABBMin; // worldspace?

    // this is (left + right << 16)
    // could maybe instead use u16 left, u16 right?
    // https://github.com/microsoft/DirectXShaderCompiler/wiki/16-Bit-Scalar-Types
    //   supported since Shader Model 6.2  
    u32 mLeftRight{};
    v3  mAABBMax;

    // an index into SceneBVH::mTLASInstancesBuffer, which is basically
    // Vector<BVHInstance> SceneBVH::mInstances
    u32 mBLAS{};
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
                       AABB32 bounds_modelspace);

    m4                 mInverseTransform {};
    m4                 mTransform        {};
    m4                 mNormalTransform  {}; // ??? unused?
    AABB32             mBounds           {}; // worldspace
    BVHMeshes::Index32 mMeshIndex        {};
    u32                mIndex            {}; // ??? index into SceneBVH::mInstances;
    //TAC_PAD_BYTES( 8 )                   {};
    Material* mMaterial{};
  };

  //static_assert_gpu_padded( BVHInstance );

  struct BVHInstances : public Vector< BVHInstance >
  {
    struct Index32{ u32 mIndex{}; };

    const BVHInstance& GetInstance( Index32 i ) const { return ( *this )[ i.mIndex ]; }
    dynmc BVHInstance& GetInstance( Index32 i ) dynmc { return ( *this )[ i.mIndex ]; }
  };


  struct TLAS
  {
    void Build();
    const TLASNode& Root() const;
    dynmc TLASNode& Root() dynmc;

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

  struct SceneIntersection
  {
    bool  IsValid() const { return mDistance < inf; }

    float                 mDistance       { inf };
    BVHInstances::Index32 mInstanceIndex  {};
    BVHTriangle::Index32  mPrimitiveIndex {};
    float                 mU              {};
    float                 mV              {};
  };

  struct BVHRay
  {
    v3 mOrigin;
    v3 mDirection;
    v3 mDirectionInv;
  };

  struct SceneBVH
  {
    SceneIntersection           IntersectTLAS( BVHRay ray_worldspace ) const;
    void                        IntersectBLAS( BVHRay ray_worldspace,
                                               u32 iInstance,
                                               SceneIntersection* ) const;

    static SceneBVH*            CreateBVH( const World*, Errors& );
    static Render::BufferHandle CreateBuffer( int, const void*, int, const char*, Errors& );



    // ----------------------------------------------------------------------------------------
#if 0
    void                        CreateBuffers( Errors& );
    void CreateAllTrianglesBuffer( Errors& );
    void CreateAllTrianglesExBuffer( Errors& );
    void CreateAllBVHNodesBuffer( Errors& );
    void CreateAllTriangleIndicesBuffer( Errors& );
    void CreateIndexDataBuffer( Errors& );
    void CreateTLASInstancesBuffer( Errors& );
    void CreateTLASNodeBuffer( Errors& );
#endif
    // --------------------------------------------------------------------------

    TLAS                           mTLAS;
    BVHMeshes                      mMeshes;
    BVHInstances                   mInstances;
    Vector< BVHNode >              mAllBVHNodes;        // gpu --> mAllBVHNodesBuffer

    Render::BufferHandle mAllBVHNodesBuffer;         // cpu --> mAllBVHNodes
    Render::BufferHandle mTLASInstancesBuffer;       // cpu --> mTLAS.mBLAS / mInstances?
    Render::BufferHandle mTLASNodeBuffer;            // cpu --> mTLAS.mNodes
  };

  struct SceneBVHDebug
  {
    void DebugImguiSceneBVH( SceneBVH* );
    void DebugVisualizeSceneBVH( Debug3DDrawData*, SceneBVH* );

  private:
    const BVHMesh* FindSelectedMesh( SceneBVH* );
    const BVHNode* FindSelectedNode( const BVHMesh* );

    void DebugImguiSceneBVHMeshes( SceneBVH* );
    void DebugImguiSceneBVHMesh( const BVHMesh* );
    void DebugImguiSceneBVHNode( const BVHNode* );

    void DebugVisualizeSceneBVHMesh( Debug3DDrawData*, const BVHMesh* );

    int iMesh            { -1 };
    int iSelectedBVHNode { -1 };
  };

} // namespace Tac

