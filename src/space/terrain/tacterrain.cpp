#include "space/physics/tacphysics.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/terrain/tacterrain.h"
#include "space/terrain/tacterraindebug.h"

#include "common/graphics/tacDebug3D.h"
#include "common/tacJson.h"
#include "common/tacMemory.h"
#include "common/thirdparty/stb_image.h"

static void TacTerrainLoadPrefab( TacJson& terrainJson, TacComponent* component )
{
    terrainJson[ "ogga" ] = "booga";
}


static void TacTerrainSavePrefab( TacJson& json, TacComponent* component )
{
}

TacComponentRegistryEntry* TacTerrain::GetEntry() { return TacTerrain::TerrainComponentRegistryEntry; }
TacTerrain* TacTerrain::GetComponent( TacEntity* entity )
{
  return ( TacTerrain* )entity->GetComponent( TacTerrain::TerrainComponentRegistryEntry );
}
static void TacTerrainLoadPrefab( TacJson* json, TacComponent* component )
{
}
static void TacTerrainSavePrefab( TacJson* json, TacComponent* component )
{
}


TacComponentRegistryEntry* TacTerrain::TerrainComponentRegistryEntry;
static TacComponent* TacCreateTerrainComponent( TacWorld* world )
{
  return TacPhysics::GetSystem( world )->CreateTerrain();
}

static  void TacDestroyTerrainComponent( TacWorld* world, TacComponent* component )
{
  TacPhysics::GetSystem( world )->DestroyTerrain( ( TacTerrain* )component );
};

void TacTerrain::TacSpaceInitPhysicsTerrain()
{
  TacTerrain::TerrainComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacTerrain::TerrainComponentRegistryEntry->mName = "Terrain";
  TacTerrain::TerrainComponentRegistryEntry->mCreateFn = TacCreateTerrainComponent;
  TacTerrain::TerrainComponentRegistryEntry->mDestroyFn = TacDestroyTerrainComponent;
  TacTerrain::TerrainComponentRegistryEntry->mNetworkBits = {};
  TacTerrain::TerrainComponentRegistryEntry->mDebugImguiFn = TacTerrainDebugImgui;
  TacTerrain::TerrainComponentRegistryEntry->mLoadFn = TacTerrainLoadPrefab;
  TacTerrain::TerrainComponentRegistryEntry->mSaveFn = TacTerrainSavePrefab;
}


void TacTerrain::LoadTestHeightmap()
{
  if( mTestHeightmapImageMemory.size() )
    return; // already loaded

  if( mTestHeightmapLoadErrors.size() )
    return; // tried to load already, but load failed


  TacVector< char > imageMemory = TacTemporaryMemory( heightmapPath, mTestHeightmapLoadErrors );
  if( mTestHeightmapLoadErrors.size() )
    return;

  int imageWidth;
  int imageHeight;
  stbi_uc* loaded = stbi_load_from_memory(
    ( stbi_uc const * )imageMemory.data(),
    imageMemory.size(),
    &imageWidth,
    &imageHeight,
    nullptr,
    STBI_grey );

  int byteCount = imageWidth * imageHeight;
  mTestHeightmapImageMemory.resize( byteCount );

  TacMemCpy( mTestHeightmapImageMemory.data(), loaded, byteCount );

  mTestHeightmapWidth = imageWidth;
  mTestHeightmapHeight = imageHeight;
  stbi_image_free( loaded );
}


v3 TacTerrain::GetVal( int x, int y )
{
  return mGrid[ x + y * this->squareVertexCount ];
};
void TacTerrain::PopulateGrid()
{
  TacTerrain* terrain = this;


  if( terrain->mGrid.empty() )
  {
    float totalGridSideLength = 50.0f;
    float width = totalGridSideLength;
    float height = totalGridSideLength;
    for( int iRow = 0; iRow < squareVertexCount; ++iRow )
    {
      for( int iCol = 0; iCol < squareVertexCount; ++iCol )
      {
        float xPercent = ( float )iRow / ( float )( squareVertexCount - 1 );
        float zPercent = ( float )iCol / ( float )( squareVertexCount - 1 );

        int heightmapX = ( int )( xPercent * ( mTestHeightmapWidth - 1 ) );
        int heightmapY = ( int )( zPercent * ( mTestHeightmapHeight - 1 ) );
        uint8_t heightmapValue = mTestHeightmapImageMemory[ heightmapX + heightmapY * mTestHeightmapWidth ];
        float heightmapPercent = heightmapValue / 255.0f;

        v3 pos;
        pos.x = xPercent * width;
        pos.y = heightmapPercent * 25.0f;
        pos.z = zPercent * height;
        terrain->mGrid.push_back( pos );
      }
    }
  }
}

void TacTerrain::DebugDraw()
{
  TacWorld* world = mEntity->mWorld;
  TacDebug3DDrawData* debug3DDrawData = world->mDebug3DDrawData;
  v3 gridColor = { 0, 0, 0 };

  for( int iRow = 0; iRow < squareVertexCount; ++iRow )
  {
    for( int iCol = 0; iCol < squareVertexCount; ++iCol )
    {
      const v3& topLeft = GetVal( iRow, iCol );

      if( iCol + 1 < squareVertexCount )
      {
        const v3& topRight = GetVal( iRow, iCol + 1 );
        debug3DDrawData->DebugDrawLine( topLeft, topRight, gridColor );
      }
      if( iRow + 1 < squareVertexCount )
      {
        const v3& bottomLeft = GetVal( iRow + 1, iCol );
        debug3DDrawData->DebugDrawLine( topLeft, bottomLeft, gridColor );
      }
      if( iCol + 1 < squareVertexCount && iRow + 1 < squareVertexCount )
      {
        const v3& bottomRight = GetVal( iRow + 1, iCol + 1 );
        debug3DDrawData->DebugDrawLine( topLeft, bottomRight, gridColor );
      }
    }
  }
}
