#include "tac_jppt_Scene.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::gpupt
{

  m4  Instance::GetModelMatrixInv() const
  {
    return m4::TransformInverse( mScale, mRotation, mPosition );
  }

  m4  Instance::GetModelMatrix() const
  {
    return m4::Transform( mScale, mRotation, mPosition );
  }

  void Shape::CalculateNormals()
  {
    const int nVerts{ mPositions.size() };
    const int nTris{ mTriangles.size() };
    mNormals.resize( nVerts );
    for( int j {}; j < nTris; j++ )
    {
      const v3i tri { mTriangles[ j ] };
      const v3 v0 { mPositions[ tri.x ] };
      const v3 v1 { mPositions[ tri.y ] };
      const v3 v2 { mPositions[ tri.z ] };
      const v3 normal { Normalize( Cross( v1 - v0, v2 - v0 ) ) };
      mNormals[ tri.x ] = normal;
      mNormals[ tri.y ] = normal;
      mNormals[ tri.z ] = normal;
    }
  }


  void Shape::CalculateTangents()
  {
    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping

    const int nVerts{ mPositions.size() };
    const int nTexCoords{ mTexCoords.size() };
    const int nTris{ mTriangles.size() };

    Vector< v3 > tan0( nVerts, {} );
    Vector< v3 > tan1( nVerts, {} );
    mTangents.resize( nVerts );
    if( nTexCoords != nVerts )
      return;

    for( int i{}; i < nTris; ++i )
    {
      //
      //    B
      //    ^
      //    |
      //    +------------------+
      //    |                  |
      // p2 ._____             |
      //    |\    \_____       |
      //    | \         \_____ |
      //    |  \              \. p1
      //    |   \            _/|
      //    |    \         _/  |
      //    |     \      _/    |
      //    |      \   _/      |
      //    |       \ /        |
      //    +--------.---------+--> T
      //            p0
      //
      // du1 = p1.u - p0.u
      // du2 = p2.u - p0.u
      // dv1 = p1.v - p0.v
      // dv2 = p2.v - p0.v
      //
      // E1 = p1.xyz - p0.xyz
      // E2 = p2.xyz - p0.xyz
      //
      // E1 = du1 * T + dv1 * B
      // E2 = du2 * T + dv2 * B
      //
      // [ E1x E1y E1z ] = [ du1 dv1 ] [ Tx Ty Tz ]
      // [ E2x E2y E2z ]   [ du2 dv2 ] [ Bx By Bz ]
      //
      //  [ Tx Ty Tz ] =             1             [ dv2 -dv1 ] [ E1x E1y E1x ]
      //  [ Bx By Bz ]   ------------------------  [ -du2 du1 ] [ E2x E2y E2z ]
      //                 ( du1 * dv2 - du2 * dv1 )

      const v3i tri{ mTriangles[ i ] };
      const int i0{ tri[ 0 ] };
      int const i1{ tri[ 1 ] };
      const int i2{ tri[ 2 ] };

      const v3 p0{ mPositions[ i0 ] };
      const v3 p1{ mPositions[ i1 ] };
      const v3 p2{ mPositions[ i2 ] };

      const v2 uv0{ mTexCoords[ i0 ] };
      const v2 uv1{ mTexCoords[ i1 ] };
      const v2 uv2{ mTexCoords[ i2 ] };

      const v3 E1{ p1 - p0 };
      const v3 E2{ p2 - p0 };

      const float du1 { ( uv1 - uv0 )[ 0 ] };
      const float dv1 { ( uv1 - uv0 )[ 1 ] };
      const float du2 { ( uv2 - uv0 )[ 0 ] };
      const float dv2 { ( uv2 - uv0 )[ 1 ] };


      const float r{ 1 / ( du1 * dv2 - du2 * dv1 ) };

      const v3 T{ r * v3( Dot( v2{ dv2, -dv1 }, v2{ E1.x, E2.x } ),
                          Dot( v2{ dv2, -dv1 }, v2{ E1.y, E2.y } ),
                          Dot( v2{ dv2, -dv1 }, v2{ E1.z, E2.z } ) ) };
      const v3 B{ r * v3( Dot( v2{ -du2, du1 }, v2{ E1.x, E2.x } ),
                          Dot( v2{ -du2, du1 }, v2{ E1.y, E2.y } ),
                          Dot( v2{ -du2, du1 }, v2{ E1.z, E2.z } ) ) };

      // += instead of = because it will be normalized later to compute an average
      tan0[ i0 ] += T;
      tan0[ i1 ] += T;
      tan0[ i2 ] += T;

      tan1[ i0 ] += B;
      tan1[ i1 ] += B;
      tan1[ i2 ] += B;
    }

    for( int i{}; i < nVerts; ++i )
    {
      const v3 n{ mNormals[ i ] };
      const v3 t{ tan0[ i ] };
      mTangents[ i ] = v4( Normalize( t - n * Dot( n, t ) ), 1 );
      mTangents[ i ].w = Dot( Cross( n, t ), tan1[ i ] ) < 0.0f ? -1.0f : 1.0f;
    }
  }

  Scene* Scene::CreateCornellBox(Errors& errors)
  {
    Scene* scene { TAC_NEW Scene };

    const Camera camera
    {
      .mFrame    { m4::Translate( { 0, 1, 3.9f } ) },
      .mLens     { 0.035f },
      .mFilm     { 0.024f },
      .mAspect   { 1.0f },
      .mFocus    { 3.9f },
      .mAperture { 0.0f },
    };
    scene->mCameras.push_back( camera );
    scene->mCameraNames.push_back( "Main Camera" );

    Shape& floor { scene->mShapes.emplace_back() };
    floor.mPositions = { {-1, 0, 1}, {1, 0, 1}, {1, 0, -1}, {-1, 0, -1} };
    floor.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    floor.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    floor.mColors.resize(floor.mPositions.size(),  {0.5,0.5,0.5,1} );
    Instance& floorInstance { scene->mInstances.emplace_back() };
    floorInstance.mShape = ( int )scene->mShapes.size() - 1;
    scene->mShapeNames.push_back( "Floor" );
    scene->mInstanceNames.push_back( "Floor" );

    Shape& mCeilingShape = scene->mShapes.emplace_back();
    mCeilingShape.mPositions = { {-1, 2, 1}, {-1, 2, -1}, {1, 2, -1}, {1, 2, 1} };
    mCeilingShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    mCeilingShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    mCeilingShape.mColors.resize( mCeilingShape.mPositions.size(), {0.6f,0.6f,0.6f,1} );
    Instance& ceilingInstance = scene->mInstances.emplace_back();
    ceilingInstance.mShape = ( int )scene->mShapes.size() - 1;
    scene->mShapeNames.push_back( "mCeiling" );
    scene->mInstanceNames.push_back( "mCeiling" );

    Shape& backWallShape { scene->mShapes.emplace_back() };
    backWallShape.mPositions = { {-1, 0, -1}, {1, 0, -1}, {1, 2, -1}, {-1, 2, -1} };
    backWallShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    backWallShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    backWallShape.mColors.resize( backWallShape.mPositions.size(), {0.4f,0.4f,0.4f,1} );
    Instance& backWallInstance = scene->mInstances.emplace_back();
    backWallInstance.mShape = ( int )scene->mShapes.size() - 1;
    scene->mShapeNames.push_back( "BackWall" );
    scene->mInstanceNames.push_back( "BackWall" );

    Shape& rightWallShape { scene->mShapes.emplace_back() };
    rightWallShape.mPositions = { {1, 0, -1}, {1, 0, 1}, {1, 2, 1}, {1, 2, -1} };
    rightWallShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    rightWallShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    rightWallShape.mColors.resize(rightWallShape.mPositions.size(), {1,0,0,1});
    Instance& rightWallInstance = scene->mInstances.emplace_back();
    rightWallInstance.mShape = ( int )scene->mShapes.size() - 1;
    scene->mShapeNames.push_back( "RightWall" );
    scene->mInstanceNames.push_back( "RightWall" );

    Shape& leftWallShape { scene->mShapes.emplace_back() };
    leftWallShape.mPositions = { {-1, 0, 1}, {-1, 0, -1}, {-1, 2, -1}, {-1, 2, 1} };
    leftWallShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    leftWallShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    leftWallShape.mColors.resize(leftWallShape.mPositions.size(), {0,1,0,1});
    Instance& leftWallInstance { scene->mInstances.emplace_back() };
    leftWallInstance.mShape = ( int )scene->mShapes.size() - 1;
    scene->mShapeNames.push_back( "LeftWall" );
    scene->mInstanceNames.push_back( "LeftWall" );

    Shape& shortBoxShape = scene->mShapes.emplace_back();
    shortBoxShape.mPositions =
    {
      {0.53f, 0.6f, 0.75f}, {0.7f, 0.6f, 0.17f},
      {0.13f, 0.6f, 0.0f}, {-0.05f, 0.6f, 0.57f}, {-0.05f, 0.0f, 0.57f},
      {-0.05f, 0.6f, 0.57f}, {0.13f, 0.6f, 0.0f}, {0.13f, 0.0f, 0.0f},
      {0.53f, 0.0f, 0.75f}, {0.53f, 0.6f, 0.75f}, {-0.05f, 0.6f, 0.57f},
      {-0.05f, 0.0f, 0.57f}, {0.7f, 0.0f, 0.17f}, {0.7f, 0.6f, 0.17f},
      {0.53f, 0.6f, 0.75f}, {0.53f, 0.0f, 0.75f}, {0.13f, 0.0f, 0.0f},
      {0.13f, 0.6f, 0.0f}, {0.7f, 0.6f, 0.17f}, {0.7f, 0.0f, 0.17f},
      {0.53f, 0.0f, 0.75f}, {0.7f, 0.0f, 0.17f}, {0.13f, 0.0f, 0.0f},
      {-0.05f, 0.0f, 0.57f}
    };
    shortBoxShape.mTriangles =
    {
      {0, 1, 2},
      {2, 3, 0},
      {4, 5, 6},
      {6, 7, 4},
      {8, 9, 10},
      {10, 11, 8},
      {12, 13, 14},
      {14, 15, 12},
      {16, 17, 18},
      {18, 19, 16},
      {20, 21, 22},
      {22, 23, 20}
    };
    shortBoxShape.mColors.resize( shortBoxShape.mPositions.size(), { 0.7f,0.7f,0.7f,1 } );
    Instance& shortBoxInstance = scene->mInstances.emplace_back();
    shortBoxInstance.mShape = ( int )scene->mShapes.size() - 1;
    scene->mShapeNames.push_back( "ShortBox" );
    scene->mInstanceNames.push_back( "ShortBox" );

    Shape& tallBoxShape = scene->mShapes.emplace_back();
    tallBoxShape.mPositions =
    {
      {-0.53f, 1.2f, 0.09f}, {0.04f, 1.2f, -0.09f},
      {-0.14f, 1.2f, -0.67f}, {-0.71f, 1.2f, -0.49f}, {-0.53f, 0.0f, 0.09f},
      {-0.53f, 1.2f, 0.09f}, {-0.71f, 1.2f, -0.49f}, {-0.71f, 0.0f, -0.49f},
      {-0.71f, 0.0f, -0.49f}, {-0.71f, 1.2f, -0.49f}, {-0.14f, 1.2f, -0.67f},
      {-0.14f, 0.0f, -0.67f}, {-0.14f, 0.0f, -0.67f}, {-0.14f, 1.2f, -0.67f},
      {0.04f, 1.2f, -0.09f}, {0.04f, 0.0f, -0.09f}, {0.04f, 0.0f, -0.09f},
      {0.04f, 1.2f, -0.09f}, {-0.53f, 1.2f, 0.09f}, {-0.53f, 0.0f, 0.09f},
      {-0.53f, 0.0f, 0.09f}, {0.04f, 0.0f, -0.09f}, {-0.14f, 0.0f, -0.67f},
      {-0.71f, 0.0f, -0.49f}
    };
    tallBoxShape.mTriangles =
    {
      {0, 1, 2},
      {2, 3, 0},
      {4, 5, 6},
      {6, 7, 4},
      {8, 9, 10},
      {10, 11, 8},
      {12, 13, 14},
      {14, 15, 12},
      {16, 17, 18},
      {18, 19, 16},
      {20, 21, 22},
      {22, 23, 20}
    };
    tallBoxShape.mColors.resize( shortBoxShape.mPositions.size(), { 0.6f,0.6f,0.6f,1 } );
    Instance& tallBoxInstance = scene->mInstances.emplace_back();
    tallBoxInstance.mShape = ( int )scene->mShapes.size() - 1;
    scene->mShapeNames.push_back( "TallBox" );
    scene->mInstanceNames.push_back( "TallBox" );

    // Checkup
    for( int i {}; i < scene->mShapes.size(); i++ )
    {
      Shape& shape{ scene->mShapes[ i ] };
      const int nVerts{ shape.mPositions.size() };
      if( shape.mNormals.empty() )
        shape.CalculateNormals();

      if( shape.mTangents.empty() )
        shape.CalculateTangents();

      if( shape.mTexCoords.size() != nVerts ) 
       shape.mTexCoords.resize( nVerts );

      if( shape.mColors.size() != nVerts )
        shape.mColors.resize( nVerts, v4{ 1,1,1,1 } );
    }


    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateBufferParams bufferParams
    {
      .mByteCount     { sizeof( gpupt::Camera ) },
      .mUsage         { Render::Usage::Dynamic },
      .mBinding       { Render::Binding::ShaderResource },
      .mOptionalName  { "jppt" },
    };
    TAC_CALL_RET( scene->mCamerasBuffer = renderDevice->CreateBuffer( bufferParams, errors ) );

    return scene;
  }
} // namespace Tac::gpupt
