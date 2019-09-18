#include "space/physics/tacphysics.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/terrain/tacterrain.h"
#include "space/terrain/tacterraindebug.h"

#include "common/graphics/tacDebug3D.h"
#include "common/tacJson.h"
#include "common/tacMemory.h"
#include "common/thirdparty/stb_image.h"
#include "common/math/tacMath.h"

TacComponentRegistryEntry* TacTerrain::TerrainComponentRegistryEntry;

static void TacTerrainSavePrefab( TacJson& json, TacComponent* component )
{
  auto terrain = ( TacTerrain* )component;
  json[ "mSideVertexCount" ] = terrain->mSideVertexCount;
  json[ "mSideLength" ] = terrain->mSideLength;
  json[ "mHeight" ] = terrain->mHeight;
  json[ "mHeightmapTexturePath" ] = terrain->mHeightmapTexturePath;
}

static void TacTerrainLoadPrefab( TacJson& json, TacComponent* component )
{
  auto terrain = ( TacTerrain* )component;
  terrain->mSideVertexCount = ( int )json[ "mSideVertexCount" ].mNumber;
  terrain->mSideLength = ( float )json[ "mSideLength" ].mNumber;
  terrain->mHeight = ( float )json[ "mHeight" ].mNumber;
  terrain->mHeightmapTexturePath = json[ "mHeightmapTexturePath" ].mString;
}

static TacComponent* TacCreateTerrainComponent( TacWorld* world )
{
  return TacPhysics::GetSystem( world )->CreateTerrain();
}

static void TacDestroyTerrainComponent( TacWorld* world, TacComponent* component )
{
  TacPhysics::GetSystem( world )->DestroyTerrain( ( TacTerrain* )component );
};

void TacTerrain::TacSpaceInitPhysicsTerrain()
{
  TacTerrain::TerrainComponentRegistryEntry = TacComponentRegistry::Instance()->RegisterNewEntry();
  TacTerrain::TerrainComponentRegistryEntry->mName = "Terrain";
  TacTerrain::TerrainComponentRegistryEntry->mNetworkBits = {};
  TacTerrain::TerrainComponentRegistryEntry->mCreateFn = TacCreateTerrainComponent;
  TacTerrain::TerrainComponentRegistryEntry->mDestroyFn = TacDestroyTerrainComponent;
  TacTerrain::TerrainComponentRegistryEntry->mDebugImguiFn = TacTerrainDebugImgui;
  TacTerrain::TerrainComponentRegistryEntry->mLoadFn = TacTerrainLoadPrefab;
  TacTerrain::TerrainComponentRegistryEntry->mSaveFn = TacTerrainSavePrefab;
}

TacComponentRegistryEntry* TacTerrain::GetEntry() { return TacTerrain::TerrainComponentRegistryEntry; }

TacTerrain* TacTerrain::GetComponent( TacEntity* entity )
{
  return ( TacTerrain* )entity->GetComponent( TacTerrain::TerrainComponentRegistryEntry );
}

void TacTerrain::LoadTestHeightmap()
{
  if( mTestHeightmapImageMemory.size() )
    return; // already loaded

  if( mTestHeightmapLoadErrors.size() )
    return; // tried to load already, but load failed


  TacVector< char > imageMemory = TacTemporaryMemory( mHeightmapTexturePath, mTestHeightmapLoadErrors );
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
  return mGrid[ x + y * this->mSideVertexCount ];
};

void TacTerrain::PopulateGrid()
{
  TacTerrain* terrain = this;


  if( TacMemCmp( mEntity->mWorldTransform.data(), mWorldCreationTransform.data(), sizeof( m4 ) ) )
    terrain->mGrid.clear();

  if( !terrain->mGrid.empty() )
    return;

  if( mTestHeightmapImageMemory.empty() )
    return;

  mSideVertexCount = TacMax( mSideVertexCount, 2 );
  mPower = TacMax( mPower, 1.0f );

  mWorldCreationTransform = mEntity->mWorldTransform;

  float width = mSideLength;
  float height = mSideLength;
  for( int iRow = 0; iRow < mSideVertexCount; ++iRow )
  {
    for( int iCol = 0; iCol < mSideVertexCount; ++iCol )
    {
      float xPercent = ( float )iRow / ( float )( mSideVertexCount - 1 );
      float zPercent = ( float )iCol / ( float )( mSideVertexCount - 1 );

      int heightmapX = ( int )( xPercent * ( mTestHeightmapWidth - 1 ) );
      int heightmapY = ( int )( zPercent * ( mTestHeightmapHeight - 1 ) );
      uint8_t heightmapValue = mTestHeightmapImageMemory[ heightmapX + heightmapY * mTestHeightmapWidth ];
      float heightmapPercent = heightmapValue / 255.0f;
      heightmapPercent *= heightmapPercent;
      heightmapPercent = TacPow( heightmapPercent, mPower );

      v3 pos;
      pos.x = xPercent * width;
      pos.y = heightmapPercent * mHeight;
      pos.z = zPercent * height;

      const float halfWidth = mSideLength / 2.0f;
      pos.x -= halfWidth;
      pos.z -= halfWidth;

      //pos += mWorldCreationPoint;
      pos = ( mEntity->mWorldTransform * v4( pos, 1.0f ) ).xyz();

      terrain->mGrid.push_back( pos );
    }
  }
}

void TacTerrain::DebugDraw()
{
  TacWorld* world = mEntity->mWorld;
  TacDebug3DDrawData* debug3DDrawData = world->mDebug3DDrawData;
  v3 gridColor = { 0, 0, 0 };

  if( mGrid.empty() )
    return;

  for( int iRow = 0; iRow < mSideVertexCount; ++iRow )
  {
    for( int iCol = 0; iCol < mSideVertexCount; ++iCol )
    {
      const v3& topLeft = GetVal( iRow, iCol );

      if( iCol + 1 < mSideVertexCount )
      {
        const v3& topRight = GetVal( iRow, iCol + 1 );
        debug3DDrawData->DebugDrawLine( topLeft, topRight, gridColor );
      }
      if( iRow + 1 < mSideVertexCount )
      {
        const v3& bottomLeft = GetVal( iRow + 1, iCol );
        debug3DDrawData->DebugDrawLine( topLeft, bottomLeft, gridColor );
      }
      if( iCol + 1 < mSideVertexCount && iRow + 1 < mSideVertexCount )
      {
        const v3& bottomRight = GetVal( iRow + 1, iCol + 1 );
        debug3DDrawData->DebugDrawLine( topLeft, bottomRight, gridColor );
      }
    }
  }
}
