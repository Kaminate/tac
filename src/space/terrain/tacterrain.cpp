#include "src/space/physics/tacPhysics.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"
#include "src/space/terrain/tacTerrain.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacJson.h"
#include "src/common/tacMemory.h"
#include "src/common/thirdparty/stb_image.h"
#include "src/common/math/tacMath.h"

namespace Tac
{
  ComponentRegistryEntry* Terrain::TerrainComponentRegistryEntry;

  static void TerrainSavePrefab( Json& json, Component* component )
  {
    auto terrain = ( Terrain* )component;
    json[ "mSideVertexCount" ] = terrain->mSideVertexCount;
    json[ "mSideLength" ] = terrain->mSideLength;
    json[ "mHeight" ] = terrain->mUpwardsHeight;
    json[ "mHeightmapTexturePath" ] = terrain->mHeightmapTexturePath;
    json[ "mGroundTexturePath" ] = terrain->mGroundTexturePath;
    json[ "mNoiseTexturePath" ] = terrain->mNoiseTexturePath;
  }

  static void TerrainLoadPrefab( Json& json, Component* component )
  {
    auto terrain = ( Terrain* )component;
    terrain->mSideVertexCount = ( int )json[ "mSideVertexCount" ].mNumber;
    terrain->mSideLength = ( float )json[ "mSideLength" ].mNumber;
    terrain->mUpwardsHeight = ( float )json[ "mHeight" ].mNumber;
    terrain->mHeightmapTexturePath = json[ "mHeightmapTexturePath" ].mString;
    terrain->mGroundTexturePath = json[ "mGroundTexturePath" ].mString;
    terrain->mNoiseTexturePath = json[ "mNoiseTexturePath" ].mString;
  }

  static Component* CreateTerrainComponent( World* world )
  {
    return Physics::GetSystem( world )->CreateTerrain();
  }

  static void DestroyTerrainComponent( World* world, Component* component )
  {
    Physics::GetSystem( world )->DestroyTerrain( ( Terrain* )component );
  };

  void TerrainDebugImgui( Component* );
  void Terrain::SpaceInitPhysicsTerrain()
  {
    Terrain::TerrainComponentRegistryEntry = ComponentRegistry::Instance()->RegisterNewEntry();
    Terrain::TerrainComponentRegistryEntry->mName = "Terrain";
    Terrain::TerrainComponentRegistryEntry->mNetworkBits = {};
    Terrain::TerrainComponentRegistryEntry->mCreateFn = CreateTerrainComponent;
    Terrain::TerrainComponentRegistryEntry->mDestroyFn = DestroyTerrainComponent;
    Terrain::TerrainComponentRegistryEntry->mDebugImguiFn = TerrainDebugImgui;
    Terrain::TerrainComponentRegistryEntry->mLoadFn = TerrainLoadPrefab;
    Terrain::TerrainComponentRegistryEntry->mSaveFn = TerrainSavePrefab;
  }

  ComponentRegistryEntry* Terrain::GetEntry() { return Terrain::TerrainComponentRegistryEntry; }

  Terrain* Terrain::GetComponent( Entity* entity )
  {
    return ( Terrain* )entity->GetComponent( Terrain::TerrainComponentRegistryEntry );
  }

  void Terrain::LoadTestHeightmap()
  {
    if( mTestHeightmapImageMemory.size() )
      return; // already loaded

    if( mTestHeightmapLoadErrors )
      return; // tried to load already, but load failed


    TemporaryMemory imageMemory = TemporaryMemoryFromFile( mHeightmapTexturePath, mTestHeightmapLoadErrors );
    if( mTestHeightmapLoadErrors )
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

    MemCpy( mTestHeightmapImageMemory.data(), loaded, byteCount );

    mTestHeightmapWidth = imageWidth;
    mTestHeightmapHeight = imageHeight;
    stbi_image_free( loaded );
  }

  v3 Terrain::GetGridVal( int iRow, int iCol )
  {
    return mRowMajorGrid[ iCol + iRow * mSideVertexCount ];
  };

  void Terrain::PopulateGrid()
  {
    Terrain* terrain = this;


    if( MemCmp(
      mEntity->mWorldTransform.data(),
      mWorldCreationTransform.data(), sizeof( m4 ) ) )
    {
      Recompute();
    }


    if( !terrain->mRowMajorGrid.empty() )
      return;

    if( mTestHeightmapImageMemory.empty() )
      return;

    mSideVertexCount = Max( mSideVertexCount, 2 );
    mPower = Max( mPower, 1.0f );

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
        heightmapPercent = Pow( heightmapPercent, mPower );

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


  void Terrain::Recompute()
  {
    mRowMajorGrid.clear();

    if( mVertexBuffer.IsValid() )
    {
      Render::DestroyVertexBuffer( mVertexBuffer, TAC_STACK_FRAME );
      mVertexBuffer = Render::VertexBufferHandle();
    }

    if( mIndexBuffer.IsValid() )
    {
      Render::DestroyIndexBuffer( mIndexBuffer, TAC_STACK_FRAME );
      mIndexBuffer = Render::IndexBufferHandle();
    }
  }

}

