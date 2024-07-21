#include "tac_terrain.h" // self-inc

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/thirdparty/stb_image.h"

#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/world/tac_world.h"

namespace Tac
{
  static ComponentRegistryEntry* sRegistry;

  static void        TerrainSavePrefab( Json& json, Component* component )
  {
    auto terrain { ( Terrain* )component };
    json[ "mSideVertexCount" ].SetNumber( terrain->mSideVertexCount );
    json[ "mSideLength" ].SetNumber( terrain->mSideLength );
    json[ "mHeight" ].SetNumber( terrain->mUpwardsHeight );
    json[ "mHeightmapTexturePath" ].SetString( terrain->mHeightmapTexturePath );
    json[ "mGroundTexturePath" ].SetString( terrain->mGroundTexturePath );
    json[ "mNoiseTexturePath" ].SetString( terrain->mNoiseTexturePath );
  }

  static void        TerrainLoadPrefab( Json& json, Component* component )
  {
    auto terrain { ( Terrain* )component };
    terrain->mSideVertexCount = ( int )json[ "mSideVertexCount" ].mNumber;
    terrain->mSideLength = ( float )json[ "mSideLength" ].mNumber;
    terrain->mUpwardsHeight = ( float )json[ "mHeight" ].mNumber;
    terrain->mHeightmapTexturePath = json[ "mHeightmapTexturePath" ].mString;
    terrain->mGroundTexturePath = json[ "mGroundTexturePath" ].mString;
    terrain->mNoiseTexturePath = json[ "mNoiseTexturePath" ].mString;
  }

  static Component*  CreateTerrainComponent( World* world )
  {
    return Physics::GetSystem( world )->CreateTerrain();
  }

  static void        DestroyTerrainComponent( World* world, Component* component )
  {
    Physics::GetSystem( world )->DestroyTerrain( ( Terrain* )component );
  }

  void TerrainDebugImgui( Terrain* );

  Terrain::Terrain()
  {
    mHeightmapTexturePath = "assets/heightmaps/heightmap2.png";
  }

  void                    Terrain::SpaceInitPhysicsTerrain()
  {
    sRegistry = ComponentRegistry_RegisterComponent();
    sRegistry->mName = "Terrain";
    sRegistry->mNetVars = {};
    sRegistry->mCreateFn = CreateTerrainComponent;
    sRegistry->mDestroyFn = DestroyTerrainComponent;
    sRegistry->mDebugImguiFn = []( Component* component ){ TerrainDebugImgui( ( Terrain* )component ); };
    sRegistry->mLoadFn = TerrainLoadPrefab;
    sRegistry->mSaveFn = TerrainSavePrefab;
  }

  const ComponentRegistryEntry* Terrain::GetEntry() const { return sRegistry; }

  Terrain*                Terrain::GetComponent( Entity* entity )
  {
    return ( Terrain* )entity->GetComponent( sRegistry );
  }

  void                    Terrain::LoadTestHeightmap()
  {
    if( mTestHeightmapImageMemory.size() )
      return; // already loaded

    if( mTestHeightmapLoadErrors )
      return; // tried to load already, but load failed

    const String imageMemory { LoadAssetPath( mHeightmapTexturePath, mTestHeightmapLoadErrors ) };
    if( mTestHeightmapLoadErrors )
      return;

    int imageWidth;
    int imageHeight;
    stbi_uc* loaded{ stbi_load_from_memory( ( stbi_uc const* )imageMemory.data(),
                                             imageMemory.size(),
                                             &imageWidth,
                                             &imageHeight,
                                             nullptr,
                                             STBI_grey ) };

    const int byteCount { imageWidth * imageHeight };
    mTestHeightmapImageMemory.resize( byteCount );

    MemCpy( mTestHeightmapImageMemory.data(), loaded, byteCount );

    mTestHeightmapWidth = imageWidth;
    mTestHeightmapHeight = imageHeight;
    stbi_image_free( loaded );
  }

  int                     Terrain::GetGridIndex( int iRow, int iCol ) const
  {
    return iCol + iRow * mSideVertexCount;
  }

  v3                      Terrain::GetGridVal( int iRow, int iCol ) const
  {
    return mRowMajorGrid[ GetGridIndex( iRow, iCol ) ];
  }

  v3                      Terrain::GetGridValNormal( int iRow, int iCol ) const
  {
    return mRowMajorGridNormals[ GetGridIndex( iRow, iCol ) ];
  }

  void                    Terrain::PopulateGrid()
  {
    Terrain* terrain { this };

    if( MemCmp(
      mEntity->mWorldTransform.data(),
      mWorldLevelEditorTransform.data(), sizeof( m4 ) ) )
    {
      Recompute();
    }


    if( !terrain->mRowMajorGrid.empty() )
      return;

    if( mTestHeightmapImageMemory.empty() )
      return;

    mSideVertexCount = Max( mSideVertexCount, 2 );
    mSideVertexCount = Min( mSideVertexCount, mTestHeightmapWidth );
    mSideVertexCount = Min( mSideVertexCount, mTestHeightmapHeight );
    mPower = Max( mPower, 1.0f );

    mWorldLevelEditorTransform = mEntity->mWorldTransform;

    const int totalVertexCount { mSideVertexCount * mSideVertexCount };
    terrain->mRowMajorGrid.reserve( totalVertexCount );

    const float width { mSideLength };
    const float height { mSideLength };
    for( int iRow {}; iRow < mSideVertexCount; ++iRow )
    {
      for( int iCol {}; iCol < mSideVertexCount; ++iCol )
      {
        const float xPercent { ( float )iRow / ( float )( mSideVertexCount - 1 ) };
        const float zPercent { ( float )iCol / ( float )( mSideVertexCount - 1 ) };

        const int heightmapX { ( int )( xPercent * ( mTestHeightmapWidth - 1 ) ) };
        const int heightmapY { ( int )( zPercent * ( mTestHeightmapHeight - 1 ) ) };
        const u8 heightmapValue { mTestHeightmapImageMemory[ heightmapX + heightmapY * mTestHeightmapWidth ] };

        float heightmapPercent = heightmapValue / 255.0f;
        heightmapPercent *= heightmapPercent;
        heightmapPercent = Pow( heightmapPercent, mPower );

        v3 pos;
        pos.x = xPercent * width;
        pos.y = heightmapPercent * mUpwardsHeight;
        pos.z = zPercent * height;

        const float halfWidth { mSideLength / 2.0f };
        pos.x -= halfWidth;
        pos.z -= halfWidth;

        //pos += mWorldLevelEditorPoint;
        pos = ( mEntity->mWorldTransform * v4( pos, 1.0f ) ).xyz();

        terrain->mRowMajorGrid.push_back( pos );
      }
    }

    for( int iRow {}; iRow < mSideVertexCount; ++iRow )
    {
      for( int iCol {}; iCol < mSideVertexCount; ++iCol )
      {
        const v3 pos { GetGridVal( iRow, iCol ) };
        const v3 posR { iCol + 1 < mSideVertexCount ? GetGridVal( iRow, iCol + 1 ) : pos };
        const v3 posL { iCol - 1 >= 0               ? GetGridVal( iRow, iCol - 1 ) : pos };
        const v3 posD { iRow - 1 >= 0               ? GetGridVal( iRow - 1, iCol ) : pos };
        const v3 posU { iRow + 1 < mSideVertexCount ? GetGridVal( iRow + 1, iCol ) : pos };
        const v3 edgeX { posR - posL };
        const v3 edgeY { posD - posU };
        const v3 normal { Normalize( Cross( edgeX, edgeY ) ) };


        //const v3 normal
        //  = ( iRow >  0  && iCol > 0  )
        //  ? Normalize( v3( 1, 0, 0 ) )
        //  : Normalize( v3( 0, 0, 1 ) );

        //const v3 posL = GetGridVal( iRow, iCol );
        //const v3 posU = GetGridVal( iRow, iCol );
        //const v3 posD = GetGridVal( iRow, iCol );
        //const int rowOffset = iRow + 1 < mSideVertexCount ? 1 : -1;
        //const int colOffset = iCol + 1 < mSideVertexCount ? 1 : -1;
        //const v3 edgeRowPos = GetGridVal( iRow + rowOffset, iCol);
        //const v3 edgeColPos = GetGridVal( iRow, iCol + colOffset );
        //const v3 edgeRow = ( edgeRowPos - pos ) * float( rowOffset );
        //const v3 edgeCol = ( edgeColPos - pos ) * float( colOffset );
        //const v3 normal = Normalize( Cross( edgeCol, edgeRow ) );
        terrain->mRowMajorGridNormals.push_back( normal );
      }
    }
  }

  void                    Terrain::Recompute()
  {
    mRowMajorGrid.clear();
    mRowMajorGridNormals.clear();

    if( Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() } )
    {
      renderDevice->DestroyBuffer( mVertexBuffer );
      renderDevice->DestroyBuffer( mIndexBuffer );
      mVertexBuffer = {};
      mIndexBuffer = {};
    }
  }

#if 0
  CollideResult Collide( const Heightmap* heightmap, const Collider* collider )
  {
    TAC_UNUSED_PARAMETER( heightmap );
    TAC_UNUSED_PARAMETER( collider );
    // get all overlapping triangles
    // get the one with the deepest penetration

    CollideResult result;
    return result;
  }
#endif

} // namespace Tac

