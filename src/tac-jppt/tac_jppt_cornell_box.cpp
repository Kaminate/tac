#include "tac_jppt_cornell_box.h"

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
//#include "tac-ecs/world/tac_world.h"

namespace Tac
{

  // so the build/preprocess step can be this file, which 
  // 
  // 1) defines the vtx POS, UV, COLOR
  // 2) uses the centroid to build the bvh
  // 2) defines the tangent

  struct Vertex
  {
    v3 mPosition;
    v2 mTexCoord;
    v4 mColor;
  };

  struct Shape
  {
    void CalculateTangents();
    void CalculateNormals();

    Vector< v3 >  mPositions;
    Vector< v3 >  mNormals;
    Vector< v2 >  mTexCoords;
    Vector< v4 >  mColors;
    Vector< v4 >  mTangents;
    Vector< v3i > mTriangles;
    const char*   mName{};
  };

  void Shape::CalculateNormals()
  {
    const int nVerts{ mPositions.size() };
    const int nTris{ mTriangles.size() };
    mNormals.resize( nVerts );
    for( int j = 0; j < nTris; j++ )
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

  struct CornellBoxGLTFBuilder
  {
    void Reset()
    {
      json.Clear();
      json[ "asset" ] = Json{}[ "version" ] = "2.0";
      json[ "scene" ] = 0;
      json[ "scenes" ][ 0 ] = Json{}[ "nodes" ][ 0 ] = 0;
      rootNode = &json[ "nodes" ][ 0 ];
      ( *rootNode )[ "name" ] = "cornellbox";

      mBinaryBlob.clear();
      //AddBinaryU32( 0x46546C67 );
      AddBinaryU8( 'g' );
      AddBinaryU8( 'l' );
      AddBinaryU8( 't' );
      AddBinaryU8( 'f' );
      AddBinaryU32( 2 ); // ver
      int iLen = AddBinaryU32( 0 ); // length ( filled in later )
    }

    void AddNode( const Shape& shape, const char* name )
    {
      Json* node{ json[ "nodes" ].AddChild() };
      ( *node )[ "name" ] = name;
    }

    void AddBinaryU8( u8 u )
    {
      mBinaryBlob.emplace_back( u );
    }

    int AddBinaryU32( u32 u )
    {
      int result = mBinaryBlob.size();
      u8* pu8{ reinterpret_cast< u8* >( &u ) };
      mBinaryBlob.emplace_back( pu8[ 0 ] );
      mBinaryBlob.emplace_back( pu8[ 1 ] );
      mBinaryBlob.emplace_back( pu8[ 2 ] );
      mBinaryBlob.emplace_back( pu8[ 3 ] );
      return result;
    }

    Vector< u8 > mBinaryBlob;

    Json json;
    Json* rootNode;
  };

  CornellBoxGLTFBuilder sBuilder;

  static Shape GetFloor()
  {
    Shape floor;
    floor.mPositions = { {-1, 0, 1}, {1, 0, 1}, {1, 0, -1}, {-1, 0, -1} };
    floor.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    floor.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    floor.mColors.resize( floor.mPositions.size(), { 0.5,0.5,0.5,1 } );
    floor.mName = "floor";
    return floor;
  }

  static Shape GetCeiling()
  {
    Shape ceiling;
    ceiling.mPositions = { {-1, 2, 1}, {-1, 2, -1}, {1, 2, -1}, {1, 2, 1} };
    ceiling.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    ceiling.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    ceiling.mColors.resize( ceiling.mPositions.size(), { 0.6f,0.6f,0.6f,1 } );
    ceiling.mName = "ceiling";
    return ceiling;
  }

  static Shape GetBackWall()
  {
    Shape backWallShape;
    backWallShape.mPositions = { {-1, 0, -1}, {1, 0, -1}, {1, 2, -1}, {-1, 2, -1} };
    backWallShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    backWallShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    backWallShape.mColors.resize( backWallShape.mPositions.size(), { 0.4f,0.4f,0.4f,1 } );
    backWallShape.mName = "backWall";
    return backWallShape;
  }

  static Shape GetRightWall()
  {
    Shape rightWallShape;
    rightWallShape.mPositions = { {1, 0, -1}, {1, 0, 1}, {1, 2, 1}, {1, 2, -1} };
    rightWallShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    rightWallShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    rightWallShape.mColors.resize( rightWallShape.mPositions.size(), { 1,0,0,1 } );
    rightWallShape.mName = "rightWall";
    return rightWallShape;
  }

  static Shape GetLeftWall()
  {
    Shape leftWallShape;
    leftWallShape.mPositions = { {-1, 0, 1}, {-1, 0, -1}, {-1, 2, -1}, {-1, 2, 1} };
    leftWallShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
    leftWallShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
    leftWallShape.mColors.resize( leftWallShape.mPositions.size(), { 0,1,0,1 } );
    leftWallShape.mName = "leftWall";
    return leftWallShape;
  }

  static Shape GetShortBox()
  {
    Shape shortBoxShape;
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
    shortBoxShape.mName = "shortBox";
    return shortBoxShape;
  }

  static Shape GetTallBox()
  {
    Shape tallBoxShape;
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
    tallBoxShape.mColors.resize( tallBoxShape.mPositions.size(), { 0.6f,0.6f,0.6f,1 } );
    tallBoxShape.mName = "tallBox";
    return tallBoxShape;
  }

  void CornellBox::DebugImGui()
  {
    if( ImGuiButton( "Save GLTF" ) )
    {
      Shape shapes[]
      {
         GetFloor(),
         GetCeiling(),
         GetBackWall(),
         GetRightWall(),
         GetLeftWall(),
         GetShortBox(),
         GetTallBox(),
      };

      sBuilder.Reset();
      for( Shape& shape : shapes )
      {
        const int nVerts{ shape.mPositions.size() };
        if( shape.mNormals.empty() )
          shape.CalculateNormals();

        if( shape.mTangents.empty() )
          shape.CalculateTangents();

        if( shape.mTexCoords.size() != nVerts )
          shape.mTexCoords.resize( nVerts );

        if( shape.mColors.size() != nVerts )
          shape.mColors.resize( nVerts, v4{ 1,1,1,1 } );

        sBuilder.AddNode( shape, shape.mName );
      }

    }
  }
}
