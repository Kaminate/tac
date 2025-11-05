#include "tac_jppt_BVH.h" // self-inc

#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/entity/tac_entity.h"

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  float AABB32::Area() const
  {
    const v3 e{ mMax - mMin };
    return Square( e.x ) * Square( e.y ) * Square( e.z );
  }

  void  AABB32::Grow( const AABB32& other )
  {
    mMin = Min( mMin, other.mMin );
    mMax = Max( mMax, other.mMax );
  }

  void  AABB32::Grow( v3 p )
  {
    mMin = Min( mMin, p );
    mMax = Max( mMax, p );
  }


  // -----------------------------------------------------------------------------------------------

  bool TLASNode::IsLeaf() const { return !mLeftRight; }
  u32 TLASNode::GetLChild() const
  {
    TAC_ASSERT( !IsLeaf() );
    return 0x1111 & ( mLeftRight >> 16 );
  }

  u32 TLASNode::GetRChild() const
  {
    TAC_ASSERT( !IsLeaf() );
    return 0x1111 & mLeftRight;
  }

  AABB32 TLASNode::GetAABB() const { return AABB32{ .mMin{ mAABBMin }, .mMax{ mAABBMax }, }; }

  static float RayAABBIntersection( const BVHRay Ray, const AABB32 aabb )
  {
    TAC_ASSERT(Ray.mDirectionInv != v3{} );
    v3 AABBMin{ aabb.mMin };
    v3 AABBMax{ aabb.mMax };
    float tx1 = ( AABBMin.x - Ray.mOrigin.x ) * Ray.mDirectionInv.x;
    float tx2 = ( AABBMax.x - Ray.mOrigin.x ) * Ray.mDirectionInv.x;
    float tmin = Min( tx1, tx2 );
    float tmax = Max( tx1, tx2 );
    float ty1 = ( AABBMin.y - Ray.mOrigin.y ) * Ray.mDirectionInv.y;
    float ty2 = ( AABBMax.y - Ray.mOrigin.y ) * Ray.mDirectionInv.y;
    tmin = Max( tmin, Min( ty1, ty2 ) );
    tmax = Min( tmax, Max( ty1, ty2 ) );
    float tz1 = ( AABBMin.z - Ray.mOrigin.z ) * Ray.mDirectionInv.z;
    float tz2 = ( AABBMax.z - Ray.mOrigin.z ) * Ray.mDirectionInv.z;
    tmin = Max( tmin, Min( tz1, tz2 ) );
    tmax = Min( tmax, Max( tz1, tz2 ) );
    return ( ( tmax <= 0 ) || ( tmax < tmin ) ) ? inf : tmin;
  }
  // -----------------------------------------------------------------------------------------------

  bool BVHNode::IsLeaf() const
  {
    return mTriangleCount;
  }

  u32 BVHNode::GetLChild() const
  {
    TAC_ASSERT( !IsLeaf() );
    return mLeftChild;
  }

  u32 BVHNode::GetRChild() const
  {
    TAC_ASSERT( !IsLeaf() );
    return mLeftChild + 1;
  }

  // -----------------------------------------------------------------------------------------------

  void  BVH::Build()
  {
    const int nTris{ mMesh->mTriangles.size() };
    if( !nTris )
      return;

    mBVHNodes.resize( nTris * 2 );
    mTriangleIndices.resize( nTris );
    for( int i{}; i < nTris; ++i )
      mTriangleIndices[ i ] = i;

    mNodesUsed = 1;
    mBVHNodes[ 0 ] = BVHNode
    {
      .mFirstTriangleIndex { 0 },
      .mTriangleCount      { ( u32 )nTris },
      .mDepth              { 0 },
      .mParent             { ( u32 )-1 },
    };
    UpdateNodeBounds( 0 );
    Subdivide( 0 );
  }

  void  BVH::Refit()
  {
  }

  void  BVH::Subdivide( u32 iNode )
  {
    dynmc BVHNode& node{ mBVHNodes[ iNode ] };
    dynmc int iAxis { -1 };
    dynmc float splitPosition {};
    const float splitCost { FindBestSplitPlane( node, iAxis, splitPosition ) };
    const float noSplitCost { CalculateNodeCost( node ) };
    if( splitCost > noSplitCost )
      return;

    dynmc u32 i { node.mFirstTriangleIndex };
    dynmc u32 j { node.mFirstTriangleIndex + node.mTriangleCount - 1 };
    while( i <= j )
    {
      if( mMesh->mTriangles[ mTriangleIndices[ i ] ].mCentroid[ iAxis ] < splitPosition )
      {
        i++;
      }
      else
      {
        Swap( mTriangleIndices[ i ], mTriangleIndices[ j ] );
        j--;
      }
    }

    const u32 leftTriangleCount { i - node.mFirstTriangleIndex };
    if( !leftTriangleCount || leftTriangleCount == node.mTriangleCount )
      return;

    const u32 childDepth { node.mDepth + 1 };
    const u32 iLChild { mNodesUsed++ };
    const u32 iRChild { mNodesUsed++ };
    TAC_ASSERT( iRChild == iLChild + 1 );
    mBVHNodes[ iLChild ] = BVHNode
    {
      .mFirstTriangleIndex { node.mFirstTriangleIndex },
      .mTriangleCount      { leftTriangleCount },
      .mDepth              { childDepth },
      .mParent             { iNode },
    };
    mBVHNodes[ iRChild ] = BVHNode
    {
      .mFirstTriangleIndex { i },
      .mTriangleCount      { node.mTriangleCount - leftTriangleCount },
      .mDepth              { childDepth },
      .mParent             { iNode },
    };
    node.mLeftChild = iLChild;
    node.mTriangleCount = 0;
    UpdateNodeBounds( iLChild );
    UpdateNodeBounds( iRChild );
    Subdivide( iLChild );
    Subdivide( iRChild );
    mMaxDepth = Max( mMaxDepth, childDepth );
  }

  void  BVH::UpdateNodeBounds( u32 iNode )
  {
    BVHNode& node{ mBVHNodes[ iNode ] };
    TAC_ASSERT( node.mAABB.mMin == v3( inf ) );
    TAC_ASSERT( node.mAABB.mMax == v3( -inf ) );
    for( u32 iiTri{ node.mFirstTriangleIndex };
         iiTri < node.mFirstTriangleIndex + node.mTriangleCount;
         iiTri++ )
    {
      const u32 iTri{ mTriangleIndices[ iiTri ] };
      const BVHTriangle& tri{ mMesh->mTriangles[ iTri ] };
      node.mAABB.Grow( tri.mV0 );
      node.mAABB.Grow( tri.mV1 );
      node.mAABB.Grow( tri.mV2 );
    }
  }

  float BVH::FindBestSplitPlane( const BVHNode& Node, int& bestAxis, float& bestSplitPosition )
  {
    const int BINS { 8 };
    dynmc float BestCost { 1e30f };
    for( int CurrentAxis {}; CurrentAxis < 3; CurrentAxis++ )
    {
      dynmc float BoundsMin { 1e30f };
      dynmc float BoundsMax { -1e30f };
      for( u32 i {}; i < Node.mTriangleCount; i++ )
      {
        const u32 iiTriangle{ Node.mFirstTriangleIndex + i };
        const u32 iTriangle{ mTriangleIndices[ iiTriangle ] };
        const BVHTriangle& triangle{ mMesh->mTriangles[ iTriangle ] };
        BoundsMin = Min( BoundsMin, triangle.mCentroid[ CurrentAxis ] );
        BoundsMax = Max( BoundsMax, triangle.mCentroid[ CurrentAxis ] );
      }
      if( BoundsMin == BoundsMax )
        continue;


      Bin bins[ BINS ];
      float Scale { BINS / ( BoundsMax - BoundsMin ) };
      for( u32 i {}; i < Node.mTriangleCount; i++ )
      {
        BVHTriangle& triangle { mMesh->mTriangles[ mTriangleIndices[ Node.mFirstTriangleIndex + i ] ] };
        int BinIndex { Min( BINS - 1, ( int )( ( triangle.mCentroid[ CurrentAxis ] - BoundsMin ) * Scale ) ) };
        bins[ BinIndex ].mTriangleCount++;
        bins[ BinIndex ].mBounds.Grow( triangle.mV0 );
        bins[ BinIndex ].mBounds.Grow( triangle.mV1 );
        bins[ BinIndex ].mBounds.Grow( triangle.mV2 );
      }

      float LeftArea[ BINS - 1 ]{};
      float RightArea[ BINS - 1 ]{};
      int LeftCount[ BINS - 1 ]{};
      int RightCount[ BINS - 1 ]{};

      AABB32 LeftBox{};
      AABB32 RightBox{};
      int LeftSum {};
      int RightSum {};

      for( int i = 0; i < BINS - 1; i++ )
      {
        //Info from the left to the right
        LeftSum += bins[ i ].mTriangleCount;
        LeftCount[ i ] = LeftSum; //Number of primitives to the right of this plane
        LeftBox.Grow( bins[ i ].mBounds );
        LeftArea[ i ] = LeftBox.Area(); //Area to the right of this plane

        //Info from the right to the left
        RightSum += bins[ BINS - 1 - i ].mTriangleCount;
        RightCount[ BINS - 2 - i ] = RightSum; //Number of primitives to the left of this plane
        RightBox.Grow( bins[ BINS - 1 - i ].mBounds );
        RightArea[ BINS - 2 - i ] = RightBox.Area(); //Area to the left of this plane
      }

      Scale = ( BoundsMax - BoundsMin ) / BINS;
      for( int i = 0; i < BINS - 1; i++ )
      {
        float PlaneCost { LeftCount[ i ] * LeftArea[ i ] + RightCount[ i ] * RightArea[ i ] };
        if( PlaneCost < BestCost )
        {
          bestAxis = CurrentAxis;
          bestSplitPosition = BoundsMin + Scale * ( i + 1 );
          BestCost = PlaneCost;
        }
      }
    }
    return BestCost;
  }

  // This functions evaluates splitting a single BVHNode into two children along the given axis and
  // position, and returns the combined values of BVH::CalculateNodeCost() on those children.
  //
  // note: SAH stands for Surface Area Hueristic
  float BVH::EvaluateSAH( const BVHNode& Node, int Axis, float Position )
  {
    AABB32 leftBox, rightBox;
    int leftCount = 0, rightCount = 0;
    for( u32 i = 0; i < Node.mTriangleCount; i++ )
    {
      BVHTriangle& triangle = mMesh->mTriangles[ mTriangleIndices[ Node.mFirstTriangleIndex + i ] ];
      if( triangle.mCentroid[ Axis ] < Position )
      {
        leftCount++;
        leftBox.Grow( triangle.mV0 );
        leftBox.Grow( triangle.mV1 );
        leftBox.Grow( triangle.mV2 );
      }
      else
      {
        rightCount++;
        rightBox.Grow( triangle.mV0 );
        rightBox.Grow( triangle.mV1 );
        rightBox.Grow( triangle.mV2 );
      }
    }

    float cost
      = leftCount * leftBox.Area()
      + rightCount * rightBox.Area();

    return cost > 0 ? cost : 1e30f;
  }

  float BVH::CalculateNodeCost( const BVHNode& node )
  {
    float area{ node.mAABB.Area() };
    float cost{ area * node.mTriangleCount };
    return cost;
  }

  // -----------------------------------------------------------------------------------------------


  void BVHMesh::SetShape( const Model* shape )
  {
    Mesh* mesh{};
    while( !mesh )
    {
      Errors errors;
      const ModelAssetManager::Params getMeshParams
      {
        .mPath       { shape->mModelPath },
        .mModelIndex { shape->mModelIndex },
      };
      mesh = ModelAssetManager::GetMesh( getMeshParams, errors );
      TAC_ASSERT( !errors );
      OS::OSThreadSleepMsec( 1 );
    }

    JPPTCPUMeshData& jpptCPUMeshData{ mesh->mJPPTCPUMeshData };
    const int nTris{ jpptCPUMeshData.mIndexes.size() / 3 };
    Vector< v3i > tris( nTris );
    for( int iTri{}; iTri < nTris; ++iTri )
    {
      const int i0{ jpptCPUMeshData.mIndexes[ iTri * 3 + 0 ] };
      const int i1{ jpptCPUMeshData.mIndexes[ iTri * 3 + 1 ] };
      const int i2{ jpptCPUMeshData.mIndexes[ iTri * 3 + 2 ] };
      tris[ iTri ] = v3i( i0, i1, i2 );
    }

    const Vector< v3 >& positions{ jpptCPUMeshData.mPositions };
    const Vector< v3 >& normals{ jpptCPUMeshData.mNormals };
    const Vector< v2 >& uvs{ jpptCPUMeshData.mTexCoords };

    mTriangles.resize( nTris );
    mTrianglesExtraData.resize( nTris );

    const m4 world{ shape->mEntity->mWorldTransform };
    for( int j {}; j < nTris; j++ )
    {
      const v3i tri{ tris[ j ] };
      const int i0{ tri[ 0 ] };
      const int i1{ tri[ 1 ] };
      const int i2{ tri[ 2 ] };
      const v3 p0{ ( world * v4( positions[ i0 ], 1 ) ).xyz() };
      const v3 p1{ ( world * v4( positions[ i1 ], 1 ) ).xyz() };
      const v3 p2{ ( world * v4( positions[ i2 ], 1 ) ).xyz() };
      const v3 centroid{ ( p0 + p1 + p2 ) / 3.0f };
      mTriangles[ j ] = BVHTriangle
      {
        .mV0       { p0 },
        .mV1       { p1 },
        .mV2       { p2 },
        .mCentroid { centroid },
      };

      auto GetVertexExtraData{ [ & ]( int i )
      {
        v3 normal{};
        if( i < normals.size() )
          normal = ( world * v4( normals[ i ], 0 ) ).xyz();

        v2 uv{};
        if( i < uvs.size() )
          uv = uvs[ i ];

        v4 color{};
        v4 tangent{};

        const VertexExtraData vExtra
        {
          .mNormal  { normal },
          .mUV      { uv },
          .mColor   { color },
          .mTangent { tangent },
        };

        return vExtra;
      } };
   
      const VertexExtraData v0Extra{ GetVertexExtraData( i0 ) };
      const VertexExtraData v1Extra{ GetVertexExtraData( i1 ) };
      const VertexExtraData v2Extra{ GetVertexExtraData( i2 ) };
      mTrianglesExtraData[ j ] = BVHTriangleExtraData
      {
        .mVertexExtraDatas { v0Extra, v1Extra, v2Extra },
      };
    }
  }

  // -----------------------------------------------------------------------------------------------

  void BVHInstance::SetTransform( const m4& transform,
                                  const m4& transformInv,
                                  const AABB32 aabb_modelspace)
  {
    mTransform = transform;
    mInverseTransform = transformInv;
    mNormalTransform = m4::Transpose( transformInv );
    mBounds = {};

    const v3 mini{ aabb_modelspace.mMin};
    const v3 maxi{ aabb_modelspace.mMax };
    for( int i{}; i < 8; ++i )
    {
      const v3 p{ ( transform * v4( i & 1 ? maxi.x : mini.x,
                                    i & 2 ? maxi.y : mini.y,
                                    i & 4 ? maxi.z : mini.z,
                                    1 ) ).xyz() };
      mBounds.Grow( p );
    }
  }

  // -----------------------------------------------------------------------------------------------

  void TLAS::Build()
  {
    mNodes.resize( mBLAS.size() * 2 );

    // Represents unparented nodes in TLAS::mNodes.
    //
    // Initially populated by leaf nodes
    // As parents are created, two nodes become 1 node
    Vector< int > NodeIndex( mBLAS.size() );

    int NodeIndices { ( int )mBLAS.size() };
    mNodesUsed = 1;

    // mNodes[ 0 ] is reserved for the root node.
    // It is assigned to at the end of this function.
    //
    // the next mNodes[1] to mNodes[1+ mBlas.size()] nodes map to mBlas

    for( u32 i {}; i < ( u32 )mBLAS.size(); i++ )
    {
      NodeIndex[ i ] = mNodesUsed;
      mNodes[ mNodesUsed ].mAABBMin = mBLAS[ i ].mBounds.mMin;
      mNodes[ mNodesUsed ].mAABBMax = mBLAS[ i ].mBounds.mMax;
      mNodes[ mNodesUsed ].mBLAS = i;
      mNodes[ mNodesUsed ].mLeftRight = 0; //Makes it a leaf.
      mNodesUsed++;
    }


    // A and B index into NodeIndex
    int A = 0;
    int B = FindBestMatch( NodeIndex.data(), NodeIndices, A ); //Best match for A
    while( NodeIndices > 1 )
    {
      int C = FindBestMatch( NodeIndex.data(), NodeIndices, B ); //Best match for B
      if( A == C ) //There is no better match --> Create a parent for them
      {
        int NodeIndexA = NodeIndex[ A ];
        int NodeIndexB = NodeIndex[ B ];
        TLASNode& NodeA = mNodes[ NodeIndexA ];
        TLASNode& NodeB = mNodes[ NodeIndexB ];

        TLASNode& NewNode = mNodes[ mNodesUsed ];
        NewNode.mLeftRight = NodeIndexA + ( NodeIndexB << 16 );
        NewNode.mAABBMin = Min( NodeA.mAABBMin, NodeB.mAABBMin );
        NewNode.mAABBMax = Max( NodeA.mAABBMax, NodeB.mAABBMax );

        // Replace A with the new Parent
        NodeIndex[ A ] = mNodesUsed++;

        // Swap B with the last node and pop_back
        NodeIndex[ B ] = NodeIndex[ NodeIndices - 1 ];
        B = FindBestMatch( NodeIndex.data(), --NodeIndices, A );
      }
      else
      {
        A = B;
        B = C;
      }
    }

    mNodes[ 0 ] = mNodes[ NodeIndex[ A ] ];
  }

  const TLASNode& TLAS::Root() const { TAC_ASSERT( mNodesUsed ); return mNodes[0]; }
  dynmc TLASNode& TLAS::Root() dynmc { TAC_ASSERT( mNodesUsed ); return mNodes[0]; }

  // Given iiNode A, finds iiNode B which has the smallest combined bounding box with A.
  // Note that FindBestMatch( A ) does not necessarily equal FindBestMatch( B )
  int  TLAS::FindBestMatch(int* List, int N, int A)
  {
      float Smallest { inf };
      int BestB { -1 };
      for(int B{}; B< N; B++)
      {
          if(B != A)
          {
              v3 BMax = Max(mNodes[List[A]].mAABBMax, mNodes[List[B]].mAABBMax);
              v3 BMin = Min(mNodes[List[A]].mAABBMin, mNodes[List[B]].mAABBMin);
              AABB32 aabb{ .mMin{ BMin }, .mMax{ BMax } };
              float Area = aabb.Area();
              if(Area < Smallest) 
              {
                  Smallest = Area;
                  BestB = B;
              }
          }
      }

      return BestB;
  }


  // -----------------------------------------------------------------------------------------------

  // ???
  SceneBVH*            SceneBVH::CreateBVH( const World* world, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    SceneBVH* sceneBVH{ TAC_NEW SceneBVH };

    Vector< const Model* > models;
    for( const Entity* entity : world->mEntities )
      if( const Model * model{ Model::GetModel( entity ) } )
        models.push_back( model );

    const int nModels{ models.size() };
    sceneBVH->mMeshes.resize( nModels );
    for( int iModel{}; iModel < nModels; ++iModel )
    {
      const int iBVHMesh{ iModel };
      BVHMesh& bvhMesh{ sceneBVH->mMeshes[ iBVHMesh ] };
      const Model* model{ models[ iModel ] };
      bvhMesh.SetShape( model );
      bvhMesh.mBVH.mMesh = &bvhMesh;
      bvhMesh.mBVH.Build();

      dynmc bool worldTransformInvExists{};
      const m4 worldTransform{ model->mEntity->mWorldTransform };
      const m4 worldTransformInv{ m4::Inverse( worldTransform, &worldTransformInvExists ) };
      TAC_ASSERT( worldTransformInvExists );

      const AABB32 bounds_modelspace{ bvhMesh.mBVH.mBVHNodes[ 0 ].mAABB };

      BVHInstance bvhInstance;
      bvhInstance.SetTransform( worldTransform, worldTransformInv, bounds_modelspace );
      bvhInstance.mMeshIndex = BVHMeshes::Index32{ ( u32 )iBVHMesh };
      bvhInstance.mIndex = ( u32 )sceneBVH->mInstances.size();
      bvhInstance.mMaterial = Material::GetMaterial( model->mEntity );

      sceneBVH->mInstances.push_back( bvhInstance );
    }

    sceneBVH->mTLAS.mBLAS = Span< BVHInstance >( sceneBVH->mInstances.data(),
                                                 sceneBVH->mInstances.size() );
    sceneBVH->mTLAS.Build();

    /*

    //Build big buffers with all the shape datas inside
    int TotalTriangleCount = 0;
    int TotalIndicesCount = 0;
    int TotalBVHNodes = 0;
    for( int i = 0; i < Result->mMeshes.size(); i++ )
    {
      BVHMesh& mesh{ Result->mMeshes[ i ] };
      TotalTriangleCount += mesh.mTriangles.size();
      TotalIndicesCount += mesh.mBVH.mTriangleIndices.size();
      TotalBVHNodes += mesh.mBVH.mNodesUsed;
    }
    Result->mAllTriangles = Vector< BVHTriangle >( TotalTriangleCount );
    Result->mAllTrianglesEx = Vector< BVHTriangleExtraData >( TotalTriangleCount );
    Result->mAllTriangleIndices = Vector< u32 >( TotalIndicesCount );
    Result->mAllBVHNodes = Vector< BVHNode >( TotalBVHNodes );
    Result->mIndexData.resize( Result->mMeshes.size() );


    // Fill the buffers
    u32 RunningTriangleCount = 0;
    u32 RunningIndicesCount = 0;
    u32 RunningBVHNodeCount = 0;
    for( int i {}; i < Result->mMeshes.size(); i++ )
    {
      BVHMesh& mesh{ Result->mMeshes[ i ] };

      MemCpy( &Result->mAllTriangles[ RunningTriangleCount ],
              mesh.mTriangles.data(),
              mesh.mTriangles.size() * sizeof( BVHTriangle ) );
      
      MemCpy( &Result->mAllTrianglesEx[ RunningTriangleCount ],
              mesh.mTrianglesExtraData.data(),
              mesh.mTrianglesExtraData.size() * sizeof( BVHTriangleExtraData ) );

      MemCpy( &Result->mAllTriangleIndices[ RunningIndicesCount ],
              mesh.mBVH.mTriangleIndices.data(),
              mesh.mBVH.mTriangleIndices.size() * sizeof( u32 ) );

      MemCpy( &Result->mAllBVHNodes[ RunningBVHNodeCount ],
              mesh.mBVH.mBVHNodes.data(),
              mesh.mBVH.mNodesUsed * sizeof( BVHNode ) );

      Result->mIndexData[ i ] =
      {
          RunningTriangleCount,
          RunningIndicesCount,
          RunningBVHNodeCount,
          ( u32 )mesh.mTriangles.size()
      };

      RunningTriangleCount += ( u32 )Result->mMeshes[ i ].mTriangles.size();
      RunningIndicesCount += ( u32 )Result->mMeshes[ i ].mBVH.mTriangleIndices.size();
      RunningBVHNodeCount += ( u32 )Result->mMeshes[ i ].mBVH.mNodesUsed;
    }


    TAC_CALL_RET( sceneBVH->CreateBuffers( errors ) );
    */

    return sceneBVH;
  }

#if 0

  void                 SceneBVH::CreateAllTrianglesBuffer( Errors& errors )
  {
    TAC_CALL( mAllTrianglesBuffer = CreateBuffer( mAllTriangles.size(),
                                                  mAllTriangles.data(),
                                                  sizeof( BVHTriangle ),
                                                  "mTrianglesBuffer",
                                                  errors ) );
  }

  void                 SceneBVH::CreateAllTrianglesExBuffer( Errors& errors )
  {
    TAC_CALL( mAllTrianglesExBuffer = CreateBuffer( mAllTrianglesEx.size(),
                                                    mAllTrianglesEx.data(),
                                                    sizeof( BVHTriangleExtraData ),
                                                    "mTrianglesExBuffer",
                                                    errors ) );
  }

  void                 SceneBVH::CreateAllBVHNodesBuffer( Errors& errors )
  {
    TAC_CALL( mAllBVHNodesBuffer = CreateBuffer( mAllBVHNodes.size(),
                                                 mAllBVHNodes.data(),
                                                 sizeof( BVHNode ),
                                                 "mBVHBuffer",
                                                 errors ) );
  }

  void                 SceneBVH::CreateAllTriangleIndicesBuffer( Errors& errors )
  {


    TAC_CALL( mAllTriangleIndicesBuffer = CreateBuffer( mAllTriangleIndices.size(),
                                                        mAllTriangleIndices.data(),
                                                        sizeof( u32 ),
                                                        "mIndicesBuffer",
                                                        errors ) );
  }

  void                 SceneBVH::CreateIndexDataBuffer( Errors& errors )
  {
    TAC_CALL( mIndexDataBuffer = CreateBuffer( mIndexData.size(),
                                               mIndexData.data(),
                                               sizeof( BVHIndexData ),
                                               "mIndexDataBuffer",
                                               errors ) );

  }

  void                 SceneBVH::CreateTLASInstancesBuffer( Errors& errors )
  {
    TAC_CALL( mTLASInstancesBuffer = CreateBuffer( mTLAS.mBLAS.size(),
                                                   mTLAS.mBLAS.data(),
                                                   sizeof( BVHInstance ),
                                                   "mTLASInstancesBuffer",
                                                   errors ) );
  }

  void                 SceneBVH::CreateTLASNodeBuffer( Errors& errors )
  {
    TAC_CALL( mTLASNodeBuffer = CreateBuffer( mTLAS.mNodes.size(),
                                              mTLAS.mNodes.data(),
                                              sizeof( TLASNode ),
                                              "mTLASNodeBuffer",
                                              errors ) );
  }


  void                 SceneBVH::CreateBuffers( Errors& errors )
  {
    // BLAS
    TAC_CALL( CreateAllTrianglesBuffer( errors ) );
    TAC_CALL( CreateAllTrianglesExBuffer(errors ) );
    TAC_CALL( CreateAllBVHNodesBuffer(errors));
    TAC_CALL( CreateAllTriangleIndicesBuffer(errors));
    TAC_CALL( CreateIndexDataBuffer(errors));

    // TLAS
    TAC_CALL( CreateTLASInstancesBuffer(errors));
    TAC_CALL( CreateTLASNodeBuffer(errors));
  }

#endif

  void                 SceneBVH::IntersectBLAS( BVHRay ray_worldspace,
                                                u32 iInstance,
                                                SceneIntersection* result ) const
  {
    const BVHInstance& bvhInstance{ mInstances[ iInstance ] };


    const v3 origin_modelspace{
      ( bvhInstance.mInverseTransform * v4( ray_worldspace.mOrigin, 1 ) ).xyz() };
    const v3 direction_modelspace{
      ( bvhInstance.mInverseTransform * v4( ray_worldspace.mDirection, 0 ) ).xyz() };
    const v3 directionInv_modelspace{ 1 / direction_modelspace.x,
                                      1 / direction_modelspace.y,
                                      1 / direction_modelspace.z };

    const BVHRay ray_modelspace
    {
      .mOrigin       { origin_modelspace },
      .mDirection    { direction_modelspace },
      .mDirectionInv { directionInv_modelspace },
    };

    const BVHMesh& bvhMesh{ mMeshes.GetMesh( bvhInstance.mMeshIndex ) };
    const BVH& bvh{ bvhMesh.mBVH };

    FixedVector< u32, 64 > bvhNodeIndexes;

    bvhNodeIndexes.push_back( 0 );
    while( !bvhNodeIndexes.empty() )
    {
      const u32 bvhNodeIndex{ bvhNodeIndexes.back() };
      bvhNodeIndexes.pop_back();

      const BVHNode& bvhNode{ bvh.mBVHNodes[ bvhNodeIndex ] };

      const float dist { RayAABBIntersection( ray_modelspace, bvhNode.mAABB ) };
      if( dist >= result->mDistance )
        continue;

      if( bvhNode.IsLeaf() )
      {
        for( u32 iiTri{ bvhNode.mFirstTriangleIndex };
             iiTri < bvhNode.mFirstTriangleIndex + bvhNode.mTriangleCount;
             iiTri++ )
        {
          const u32 iTri{ bvh.mTriangleIndices[ iiTri ] };
          const BVHTriangle& tri{ bvhMesh.mTriangles[ iTri ] };

          const Ray isectRay
          {
            .mOrigin    { ray_modelspace.mOrigin },
            .mDirection { ray_modelspace.mDirection },
          };
          const Triangle isectTri{ tri.mV0, tri.mV1, tri.mV2 };
          const RayTriangle isectOut( isectRay, isectTri );
          if( isectOut.mValid && isectOut.mT < result->mDistance )
          {
            *result = SceneIntersection
            {
              .mDistance       { isectOut.mT },
              .mInstanceIndex  { iInstance },
              .mPrimitiveIndex { iTri },
              .mU              { isectOut.mU },
              .mV              { isectOut.mV },
            };
          }
        }
      }
      else
      {
        bvhNodeIndexes.push_back( bvhNode.GetLChild() );
        bvhNodeIndexes.push_back( bvhNode.GetRChild() );
      }
    }
    bvhMesh.mBVH;
  }

  SceneIntersection    SceneBVH::IntersectTLAS( BVHRay ray_worldspace ) const
  {
    if( !mTLAS.mNodesUsed )
      return {};

    const v3 unitDir{ Normalize( ray_worldspace.mDirection ) };
    const v3 dirInv{ 1 / unitDir.x, 1 / unitDir.y, 1 / unitDir.z }; // not a 
    ray_worldspace.mDirection = unitDir;
    ray_worldspace.mDirectionInv = dirInv;

    SceneIntersection result;
    
    FixedVector< u32, 64 > iTLASNodes{ 0 };
    while( !iTLASNodes.empty() )
    {
      const u32 iTLASNode{ iTLASNodes.back() };
      iTLASNodes.pop_back();

      const TLASNode& tlasNode{ mTLAS.mNodes[ iTLASNode ] };

      const AABB32 tlasNodeAABB_worldspace
      {
        .mMin{ tlasNode.mAABBMin },
        .mMax{ tlasNode.mAABBMax },
      };

      const float dist{ RayAABBIntersection( ray_worldspace, tlasNodeAABB_worldspace ) };
      if( dist >= result.mDistance )
        continue;

      if( tlasNode.IsLeaf() )
      {
        IntersectBLAS( ray_worldspace,  tlasNode.mBLAS, &result );
      }
      else
      {
        const u32 iLChild{ tlasNode.GetLChild() };
        const u32 iRChild{ tlasNode.GetRChild() };
        iTLASNodes.push_back( iLChild );
        iTLASNodes.push_back( iRChild );
      }
    }

    return result;
  }

  Render::BufferHandle SceneBVH::CreateBuffer( int numElements,
                                               const void* bytes,
                                               int stride,
                                               const char* name,
                                               Errors& errors )
  {
    const Render::CreateBufferParams bufParams
    {
      .mByteCount     { numElements * stride },
      .mBytes         { bytes },
      .mStride        { stride },
      .mUsage         { Render::Usage::Static },
      .mBinding       { Render::Binding::ShaderResource },
      .mGpuBufferMode { Render::GpuBufferMode::kStructured },
      .mOptionalName  { name },
    };
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    return renderDevice->CreateBuffer( bufParams, errors );
  }

  // -----------------------------------------------------------------------------------------------

  void           SceneBVHDebug::DebugVisualizeSceneBVHMesh( Debug3DDrawData* drawData,
                                                            const BVHMesh* bvhMesh )
  {
    if( !bvhMesh )
      return;

    Vector< const BVHNode* > nodes;

    if( const BVHNode* selectedbvhNode{ FindSelectedNode( bvhMesh ) }; selectedbvhNode )
      nodes.push_back( selectedbvhNode );
    else if( bvhMesh->mBVH.mNodesUsed )
      nodes.push_back( &bvhMesh->mBVH.mBVHNodes[0] );

    const BVH& bvh{ bvhMesh->mBVH };
    const u32 maxDepth{ bvh.mMaxDepth };
    while( !nodes.empty() )
    {
      const BVHNode* node{ nodes.back() };
      nodes.pop_back();

      const v3 color { Lerp( v3( 0.8f ), v3( 0.2f ), node->mDepth / ( maxDepth + 0.0001f ) ) };
      const v3 pad( .0f );
      drawData->DebugDraw3DAABB( node->mAABB.mMin - pad, node->mAABB.mMax + pad, color );

      if( node->IsLeaf() )
      {
        const BVHNode& bvhNode{ *node };
        for( u32 iiTri{ bvhNode.mFirstTriangleIndex };
             iiTri < bvhNode.mFirstTriangleIndex + bvhNode.mTriangleCount;
             iiTri++ )
        {
          const u32 iTri{ bvh.mTriangleIndices[ iiTri ] };
          const BVHTriangle& bvhTri{ bvhMesh->mTriangles[ iTri ] };
          const v3 triColor{ v3( 0xec, 0x97 , 0x06 ) / 255.0f };
          drawData->DebugDraw3DTriangle( bvhTri.mV0, bvhTri.mV1, bvhTri.mV2, triColor );
        }
      }
      else
      {
        const BVHNode* lChild{ &bvhMesh->mBVH.mBVHNodes[ node->GetLChild() ] };
        const BVHNode* rChild{ &bvhMesh->mBVH.mBVHNodes[ node->GetRChild() ] };
        nodes.push_back( lChild );
        nodes.push_back( rChild );
      }
    }
  }

  void           SceneBVHDebug::DebugVisualizeSceneBVH( Debug3DDrawData* drawData,
                                                        SceneBVH* sceneBVH )
  {
    if( !sceneBVH )
      return;

    const BVHMesh* mesh{ FindSelectedMesh( sceneBVH ) };
    DebugVisualizeSceneBVHMesh(  drawData, mesh );
  }

  void           SceneBVHDebug::DebugImguiSceneBVH( SceneBVH* sceneBVH )
  {
    if( !sceneBVH )
      return;

    if( !ImGuiCollapsingHeader( "SceneBVH" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
    DebugImguiSceneBVHMeshes( sceneBVH );
  }


  const BVHNode* SceneBVHDebug::FindSelectedNode( const BVHMesh* bvhMesh )
  {
    if( !bvhMesh )
      return nullptr;

    const BVH& bvh{ bvhMesh->mBVH };
    const Vector< BVHNode >& bvhNodes{ bvh.mBVHNodes };
    const u32 nNodes{ bvh.mNodesUsed };
    const bool isNodeSelected{ iSelectedBVHNode >= 0 && iSelectedBVHNode < ( int )nNodes };
    if( !isNodeSelected )
      return nullptr;

    const BVHNode* bvhNode{ &bvhNodes[ iSelectedBVHNode ] };
    return bvhNode;
  }

  const BVHMesh* SceneBVHDebug::FindSelectedMesh( SceneBVH* sceneBVH ) const
  {
    const Vector< BVHMesh >& meshes{ sceneBVH->mMeshes };
    const int nMeshes{ meshes.size() };
    const bool isMeshSelected{ iMesh >= 0 && iMesh < nMeshes };
    if( !isMeshSelected )
      return nullptr;

    const BVHMesh& mesh{ meshes[ iMesh ] };
    return &mesh;
  }

  void           SceneBVHDebug::DebugImguiSceneBVHMeshes( SceneBVH* sceneBVH)
  {
    if( !ImGuiCollapsingHeader( "Vector<BVHMesh>" ) )
      return;
    TAC_IMGUI_INDENT_BLOCK;
    const Vector< BVHMesh >& meshes{ sceneBVH->mMeshes };
    const int nMeshes{ meshes.size() };
    if( !nMeshes )
      return;

    ImGuiText( "Select mesh: " );
    for( int i{}; i < nMeshes; ++i )
    {
      ImGuiSameLine();

      if( ImGuiButton( ToString( i ) + ( i == iMesh ? "*" : "" ) ) )
        iMesh = i;
    }

    ImGuiText( "Selected mesh: " + ToString( iMesh ) );

    const bool isMeshSelected{ iMesh >= 0 && iMesh < nMeshes };
    if( !isMeshSelected )
      return;

    const BVHMesh* mesh{ FindSelectedMesh( sceneBVH ) };
    DebugImguiSceneBVHMesh( mesh );
  }

  void           SceneBVHDebug::DebugImguiSceneBVHMesh( const BVHMesh* bvhMesh )
  {
    if( !bvhMesh )
      return;

    const BVH& bvh{ bvhMesh->mBVH };
    //const Vector< BVHNode >& bvhNodes{ bvh.mBVHNodes };
    const u32 nNodes{ bvh.mNodesUsed };
    ImGuiText( "BVH Node Count : " + ToString( nNodes ) );
    ImGuiText( "Selected BVH Node: " + ToString( iSelectedBVHNode ) );
    if( nNodes && ImGuiButton( "Unselect BVH Node" ) )
      iSelectedBVHNode = -1;

    if( nNodes )
    {
      ImGuiText( "Select Node" );
      for( int i{}; i < ( int )nNodes; ++i )
      {
        ImGuiSameLine();
        if( ImGuiButton( ToString( i ) ) )
          iSelectedBVHNode = i;
      }
    }

    const BVHNode* bvhNode{ FindSelectedNode( bvhMesh ) };
    DebugImguiSceneBVHNode( bvhNode );
  }


  void           SceneBVHDebug::DebugImguiSceneBVHNode( const BVHNode* pbvhNode )
  {
    if(!pbvhNode)
      return;
    const BVHNode& bvhNode{ *pbvhNode };
    if( !bvhNode.IsLeaf() )
    {
      if( ImGuiButton( "Select L child: " + ToString( bvhNode.GetLChild() ) ) )
        iSelectedBVHNode = ( int )bvhNode.GetLChild();
      if( ImGuiButton( "Select R child: " + ToString( bvhNode.GetRChild() ) ) )
        iSelectedBVHNode = ( int )( bvhNode.GetRChild() );
    }
  }

} // namespace Tac

