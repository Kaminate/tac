#include "tac_jppt_BVH.h"

namespace Tac::gpupt
{
  float AABB32::Area() const
  {
    const v3 e{ mMax - mMin };
    return Square( e.x ) * Square( e.y ) * Square( e.z );
  }

  void AABB32::Grow( const AABB32& other )
  {
    mMin = Min( mMin, other.mMin );
    mMax = Max( mMax, other.mMax );
  }

  void AABB32::Grow( v3 p )
  {
    mMin = Min( mMin, p );
    mMax = Max( mMax, p );
  }

  bool BVHNode::IsLeaf() const
  {
    return mTriangleCount;
  }

  void BVH::Build()
  {
    const int nTris{ mMesh->mTriangles.size() };
    mBVHNodes.resize( nTris * 2 );
    mTriangleIndices.resize( nTris );
    for( int i{}; i < nTris; ++i )
      mTriangleIndices[i]=i;

    mNodesUsed = 1;
    mBVHNodes[ 0 ] = BVHNode
    {
      .mFirstTriangleIndex = 0,
      .mTriangleCount = ( u32 )nTris,
    };
    UpdateNodeBounds( 0 );
    Subdivide( 0 );
  }

  void BVH::Refit()
  {
  }

  void BVH::Subdivide( u32 iNode )
  {
    BVHNode& node{ mBVHNodes[ iNode ] };
    int iAxis = -1;
    float splitPosition = 0;
    float splitCost = FindBestSplitPlane( node, iAxis, splitPosition );
    float noSplitCost = CalculateNodeCost( node );
    if( splitCost > noSplitCost )
      return;

    u32 i = node.mFirstTriangleIndex;
    u32 j = node.mFirstTriangleIndex + node.mTriangleCount - 1;
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

    u32 leftTriangleCount = i - node.mFirstTriangleIndex;
    if( !leftTriangleCount || leftTriangleCount == node.mTriangleCount )
      return;

    int iLChild = mNodesUsed++;
    int iRChild = mNodesUsed++;
    mBVHNodes[ iLChild ] = BVHNode
    {
      .mFirstTriangleIndex{ node.mFirstTriangleIndex },
      .mTriangleCount{ leftTriangleCount },
    };
    mBVHNodes[ iRChild ] = BVHNode
    {
      .mFirstTriangleIndex{ i },
      .mTriangleCount{ node.mTriangleCount - leftTriangleCount },
    };

    node.mLeftChild = iLChild;
    node.mTriangleCount = 0;
    UpdateNodeBounds( iLChild );
    UpdateNodeBounds( iRChild );
    Subdivide( iLChild );
    Subdivide( iRChild );
  }

  void BVH::UpdateNodeBounds( u32 iNode )
  {
    BVHNode& node{ mBVHNodes[ iNode ] };
    TAC_ASSERT( node.mAABB.mMin == AABB32().mMin );
    TAC_ASSERT( node.mAABB.mMax == AABB32().mMax );
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
    const int BINS = 8;
    float BestCost = 1e30f;
    for( int CurrentAxis = 0; CurrentAxis < 3; CurrentAxis++ )
    {
      float BoundsMin = 1e30f;
      float BoundsMax = -1e30f;
      for( u32 i = 0; i < Node.mTriangleCount; i++ )
      {
        BVHTriangle& triangle = mMesh->mTriangles[ mTriangleIndices[ Node.mFirstTriangleIndex + i ] ];
        BoundsMin = Min( BoundsMin, triangle.mCentroid[ CurrentAxis ] );
        BoundsMax = Max( BoundsMax, triangle.mCentroid[ CurrentAxis ] );
      }
      if( BoundsMin == BoundsMax ) continue;


      Bin bins[ BINS ];
      float Scale = BINS / ( BoundsMax - BoundsMin );
      for( u32 i = 0; i < Node.mTriangleCount; i++ )
      {
        BVHTriangle& triangle = mMesh->mTriangles[ mTriangleIndices[ Node.mFirstTriangleIndex + i ] ];
        int BinIndex = Min( BINS - 1, ( int )( ( triangle.mCentroid[ CurrentAxis ] - BoundsMin ) * Scale ) );
        bins[ BinIndex ].mTriangleCount++;
        bins[ BinIndex ].mBounds.Grow( triangle.mV0 );
        bins[ BinIndex ].mBounds.Grow( triangle.mV1 );
        bins[ BinIndex ].mBounds.Grow( triangle.mV2 );
      }

      float LeftArea[ BINS - 1 ], RightArea[ BINS - 1 ];
      int LeftCount[ BINS - 1 ], RightCount[ BINS - 1 ];

      AABB32 LeftBox, RightBox;
      int LeftSum = 0, RightSum = 0;

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
        float PlaneCost = LeftCount[ i ] * LeftArea[ i ] + RightCount[ i ] * RightArea[ i ];
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

  void BVHMesh::SetShape( const Shape& shape )
  {
    const int nTris{shape.mTriangles.size()};
    mTriangles.resize( nTris );
    mTrianglesExtraData.resize( nTris );
    for( int j = 0; j < nTris; j++ )
    {
      const v3i tri{shape.mTriangles[ j ]};
      const int i0{ tri[ 0 ] };
      const int i1{ tri[ 1 ] };
      const int i2{ tri[ 2 ] };


      mTriangles[ j ] = BVHTriangle
      {
        .mV0 = shape.mPositions[ i0 ],
        .mV1 = shape.mPositions[ i1 ],
        .mV2 = shape.mPositions[ i2 ],
        .mCentroid
        {
          ( shape.mPositions[ i0 ] + shape.mPositions[ i1 ] + shape.mPositions[ i2 ] ) / 3.0f
        },
      };

   
      const VertexExtraData v0Extra
      {
        .mNormal  { shape.mNormals[ i0 ] },
        .mUV      { shape.mTexCoords[ i0 ] },
        .mColor   { shape.mColors[ i0 ] },
        .mTangent { shape.mTangents[ i0 ] },
      };

      const VertexExtraData v1Extra
      {
        .mNormal  { shape.mNormals[ i1 ] },
        .mUV      { shape.mTexCoords[ i1 ] },
        .mColor   { shape.mColors[ i1 ] },
        .mTangent { shape.mTangents[ i1 ] },
      };

      const VertexExtraData v2Extra
      {
        .mNormal  { shape.mNormals[ i2 ] },
        .mUV      { shape.mTexCoords[ i2 ] },
        .mColor   { shape.mColors[ i2 ] },
        .mTangent { shape.mTangents[ i2 ] },
      };

      mTrianglesExtraData[ j ] = BVHTriangleExtraData
      {
        .mVertexExtraDatas { v0Extra, v1Extra, v2Extra },
      };
    }

  }

  void BVHInstance::SetTransform( const m4& transform,
                                  const m4& transformInv,
                                  AABB32 aabb)
  {
    mTransform = transform;
    mInverseTransform = transformInv;
    mNormalTransform = m4::Transpose( transformInv );
    mBounds = {};

    v3 mini{ aabb.mMin};
    v3 maxi{ aabb.mMax };
    for( int i{}; i < 8; ++i )
    {
      v3 p{ ( transform * v4( i & 1 ? maxi.x : mini.x,
                              i & 2 ? maxi.y : mini.y,
                              i & 4 ? maxi.z : mini.z,
                              1 ) ).xyz() };
      mBounds.Grow( p );
    }
  }

  void TLAS::Build()
  {
    mNodes.resize( mBLAS.size() * 2 );

    // Represents unparented nodes in TLAS::mNodes.
    //
    // Initially populated by leaf nodes
    // As parents are created, two nodes become 1 node
    Vector<int> NodeIndex( mBLAS.size() );

    int NodeIndices = ( int )mBLAS.size();
    mNodesUsed = 1;

    // mNodes[ 0 ] is reserved for the root node.
    // It is assigned to at the end of this function.
    //
    // the next mNodes[1] to mNodes[1+ mBlas.size()] nodes map to mBlas

    for( u32 i = 0; i < ( u32 )mBLAS.size(); i++ )
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

  // Given iiNode A, finds iiNode B which has the smallest combined bounding box with A.
  // Note that FindBestMatch( A ) does not necessarily equal FindBestMatch( B )
  int TLAS::FindBestMatch(int* List, int N, int A)
  {
      float Smallest = inf;
      int BestB = -1;
      for(int B=0; B< N; B++)
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


  // ???
  SceneBVH* SceneBVH::CreateBVH( const Scene* Scene, Errors& errors)
  {
    // Build the low level bvh of each mesh
    SceneBVH* Result { TAC_NEW SceneBVH};
    const int nShapes{Scene->mShapes.size()};
    Result->mMeshes.resize( nShapes );
    for( int i = 0; i < nShapes; i++ )
    {
      BVHMesh& mesh{ Result->mMeshes[ i ] };
      mesh.SetShape( Scene->mShapes[ i ] );
      mesh.mBVH.mMesh = &mesh;
      mesh.mBVH.Build();
    }

    //Build the array of instances
    const int nInstances{Scene->mInstances.size()};
    Result->mInstances.resize( nInstances );
    for( int i = 0; i < nInstances; i++ )
    {
      const Instance& instance{ Scene->mInstances[ i ] };
      const m4 transform{ instance.GetModelMatrix() };
      const m4 transformInv{ instance.GetModelMatrixInv() };
      const AABB32 aabb{ Result->mMeshes[ instance.mShape ].mBVH.mBVHNodes[ 0 ].mAABB };

      BVHInstance& bvhInstance{Result->mInstances[ i ]};
      bvhInstance.mMeshIndex = instance.mShape;
      bvhInstance.mIndex = i;
      bvhInstance.SetTransform( transform, transformInv, aabb );
    }

    // Build the top level data structure
    Result->mTLAS.mBLAS = Span<BVHInstance>( Result->mInstances.data(),
                                             Result->mInstances.size() );
    Result->mTLAS.Build();

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
    Result->mAllTriangles = Vector<BVHTriangle>( TotalTriangleCount );
    Result->mAllTrianglesEx = Vector<BVHTriangleExtraData>( TotalTriangleCount );
    Result->mAllTriangleIndices = Vector<u32>( TotalIndicesCount );
    Result->mAllBVHNodes = Vector<BVHNode>( TotalBVHNodes );
    Result->mIndexData.resize( Result->mMeshes.size() );


    // Fill the buffers
    u32 RunningTriangleCount = 0;
    u32 RunningIndicesCount = 0;
    u32 RunningBVHNodeCount = 0;
    for( int i = 0; i < Result->mMeshes.size(); i++ )
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


    TAC_CALL_RET( {}, Result->CreateBuffers( errors ) );

    Result->mScene = Scene;
    return Result;
  }

  void SceneBVH::CreateBuffers( Errors& errors )
  {
    // Upload to the gpu

    // BLAS
    TAC_CALL( mAllTrianglesBuffer = CreateBuffer( mAllTriangles.size(),
                                                  mAllTriangles.data(),
                                                  sizeof( BVHTriangle ),
                                                  "mTrianglesBuffer",
                                                  errors ) );

    TAC_CALL( mAllTrianglesExBuffer = CreateBuffer( mAllTrianglesEx.size(),
                                                    mAllTrianglesEx.data(),
                                                    sizeof( BVHTriangleExtraData ),
                                                    "mTrianglesExBuffer",
                                                    errors ) );

    TAC_CALL( mAllBVHNodesBuffer = CreateBuffer( mAllBVHNodes.size(),
                                                 mAllBVHNodes.data(),
                                                 sizeof( BVHNode ),
                                                 "mBVHBuffer",
                                                 errors ) );

    TAC_CALL( mAllTriangleIndicesBuffer = CreateBuffer( mAllTriangleIndices.size(),
                                                        mAllTriangleIndices.data(),
                                                        sizeof( u32 ),
                                                        "mIndicesBuffer",
                                                        errors ) );

    TAC_CALL( mIndexDataBuffer = CreateBuffer( mIndexData.size(),
                                               mIndexData.data(),
                                               sizeof( BVHIndexData ),
                                               "mIndexDataBuffer",
                                               errors ) );

    // TLAS
    TAC_CALL( mTLASInstancesBuffer = CreateBuffer( mTLAS.mBLAS.size(),
                                                   mTLAS.mBLAS.data(),
                                                   sizeof( BVHInstance ),
                                                   "mTLASInstancesBuffer",
                                                   errors ) );

    TAC_CALL( mTLASNodeBuffer = CreateBuffer( mTLAS.mNodes.size(),
                                              mTLAS.mNodes.data(),
                                              sizeof( TLASNode ),
                                              "mTLASNodeBuffer",
                                              errors ) );
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

} // namespace Tac::gpupt

