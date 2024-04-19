#pragma once

#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-ecs/component/tac_component.h"

namespace Tac
{
  struct VertexBuffer;
  struct IndexBuffer;

  struct TerrainOBB
  {
    v3 mPos;
    v3 mHalfExtents;
    v3 mEulerRads;
  };

  struct Heightmap
  {
    float mMaxHeight = 100.0f;
  };


  struct Terrain : public Component
  {
    Terrain();
    static void                    SpaceInitPhysicsTerrain();
    const ComponentRegistryEntry*  GetEntry() const override;
    static Terrain*                GetComponent( Entity* );
    void                           LoadTestHeightmap();
    void                           PopulateGrid();
    int                            GetGridIndex( int iRow, int iCol ) const;
    v3                             GetGridVal( int iRow, int iCol ) const;
    v3                             GetGridValNormal( int iRow, int iCol ) const;
    void                           Recompute();
    int                            mSideVertexCount = 50;
    float                          mSideLength = 50.0f;
    float                          mUpwardsHeight = 20.0f;
    Vector< v3 >                   mRowMajorGrid;
    Vector< v3 >                   mRowMajorGridNormals;
    AssetPathString                mHeightmapTexturePath;
    AssetPathString                mGroundTexturePath;
    AssetPathString                mNoiseTexturePath;
    int                            mTestHeightmapWidth = 0;
    int                            mTestHeightmapHeight = 0;
    float                          mPower = 1;
    Vector< unsigned char >        mTestHeightmapImageMemory;
    m4                             mWorldLevelEditorTransform = {};


    // do you think this shit should actually be owned by the game presentation?
    Render::BufferHandle     mVertexBuffer;
    Render::BufferHandle      mIndexBuffer;
    int                            mIndexCount = 0;
    Vector< TerrainOBB >           mTerrainOBBs;
    Errors                         mTestHeightmapLoadErrors;
  };


#if 0
  struct CollideResult
  {
    bool mCollided = false;

  };

  //CollideResult Collide( const Heightmap* heightmap, const Collider* collider );
#endif

}

