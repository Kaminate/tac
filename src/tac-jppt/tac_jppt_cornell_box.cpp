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

  struct CornellBoxGLTFBuilder
  {
    void Reset()
    {
      json[ "asset" ] = Json{}[ "version" ] = "2.0";
      json[ "scene" ] = 0;
      json[ "scenes" ][ 0 ] = Json{}[ "nodes" ][ 0 ] = 0;
      rootNode = &json[ "nodes" ][ 0 ];
      ( *rootNode )[ "name" ] = "cornellbox";
    }

    void AddNode( Span< v3 > positions, Span< v3i > triangles, Span< v2 > texcoords, Span)
    {
    }

    Json json;
    Json* rootNode;
  };

  CornellBoxGLTFBuilder sBuilder;

  void CornellBox::DebugImGui()
  {
    if( ImGuiButton( "Create Floor" ) )
    {
      // save as prefab?

      //World world;
      //world.SpawnEntity();

      Json leftWallNode;
      leftWallNode;

      Shape& leftWallShape { scene->mShapes.emplace_back() };
      leftWallShape.mPositions = { {-1, 0, 1}, {-1, 0, -1}, {-1, 2, -1}, {-1, 2, 1} };
      leftWallShape.mTriangles = { {0, 1, 2}, {2, 3, 0} };
      leftWallShape.mTexCoords = { {0, 1}, {1, 1}, {1, 0}, {0, 0} };
      leftWallShape.mColors.resize(leftWallShape.mPositions.size(), {0,1,0,1});
      Instance& leftWallInstance { scene->mInstances.emplace_back() };
      leftWallInstance.mShape = ( int )scene->mShapes.size() - 1;
      scene->mShapeNames.push_back( "LeftWall" );
      scene->mInstanceNames.push_back( "LeftWall" );



      Json root;


    }
  }
}
