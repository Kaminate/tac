#include "space/physics/tacphysics.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/terrain/tacterrain.h"

#include "common/graphics/tacDebug3D.h"
#include "common/graphics/tacRenderer.h"
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
  json[ "mHeight" ] = terrain->mUpwardsHeight;
  json[ "mHeightmapTexturePath" ] = terrain->mHeightmapTexturePath;
  json[ "mGroundTexturePath" ] = terrain->mGroundTexturePath;
  json[ "mNoiseTexturePath" ] = terrain->mNoiseTexturePath;
}

static void TacTerrainLoadPrefab( TacJson& json, TacComponent* component )
{
  auto terrain = ( TacTerrain* )component;
  terrain->mSideVertexCount = ( int )json[ "mSideVertexCount" ].mNumber;
  terrain->mSideLength = ( float )json[ "mSideLength" ].mNumber;
  terrain->mUpwardsHeight = ( float )json[ "mHeight" ].mNumber;
  terrain->mHeightmapTexturePath = json[ "mHeightmapTexturePath" ].mString;
  terrain->mGroundTexturePath = json[ "mGroundTexturePath" ].mString;
  terrain->mNoiseTexturePath = json[ "mNoiseTexturePath" ].mString;
}

static TacComponent* TacCreateTerrainComponent( TacWorld* world )
{
  return TacPhysics::GetSystem( world )->CreateTerrain();
}

static void TacDestroyTerrainComponent( TacWorld* world, TacComponent* component )
{
  TacPhysics::GetSystem( world )->DestroyTerrain( ( TacTerrain* )component );
};

void TacTerrainDebugImgui( TacComponent* );
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

v3 TacTerrain::GetGridVal( int iRow, int iCol )
{
  return mRowMajorGrid[ iCol + iRow * mSideVertexCount ];
};

void TacTerrain::PopulateGrid()
{
  TacTerrain* terrain = this;


  if( TacMemCmp(
    mEntity->mWorldTransform.data(),
    mWorldCreationTransform.data(), sizeof( m4 ) ) )
  {
    Recompute();
  }


  if( !terrain->mRowMajorGrid.empty() )
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
      pos.y = heightmapPercent * mUpwardsHeight;
      pos.z = zPercent * height;

      const float halfWidth = mSideLength / 2.0f;
      pos.x -= halfWidth;
      pos.z -= halfWidth;

      //pos += mWorldCreationPoint;
      pos = ( mEntity->mWorldTransform * v4( pos, 1.0f ) ).xyz();

      terrain->mRowMajorGrid.push_back( pos );
    }
  }
}


void TacTerrain::Recompute()
{
  mRowMajorGrid.clear();

  TacRenderer::Instance->RemoveRendererResource(  mVertexBuffer );
  mVertexBuffer = nullptr;

  TacRenderer::Instance->RemoveRendererResource(  mIndexBuffer );
  mIndexBuffer = nullptr;
}
