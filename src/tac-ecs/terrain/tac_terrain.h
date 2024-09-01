#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/meta/tac_meta_impl.h"

namespace Tac
{
  struct VertexBuffer;
  struct IndexBuffer;

  struct TerrainOBB
  {
    v3 mPos         {};
    v3 mHalfExtents {};
    v3 mEulerRads   {};
  };

  struct Heightmap
  {
    float mMaxHeight { 100.0f };
  };


  struct Terrain : public Component
  {
    Terrain();

    static void                    SpaceInitPhysicsTerrain();
    static Terrain*                GetComponent( Entity* );

    const ComponentInfo*           GetEntry() const override;

    void                           LoadTestHeightmap();
    void                           PopulateGrid();
    int                            GetGridIndex( int iRow, int iCol ) const;
    v3                             GetGridVal( int iRow, int iCol ) const;
    v3                             GetGridValNormal( int iRow, int iCol ) const;
    void                           Recompute();

    int                            mSideVertexCount           { 50 };
    float                          mSideLength                { 50.0f };
    float                          mUpwardsHeight             { 20.0f };
    Vector< v3 >                   mRowMajorGrid              {};
    Vector< v3 >                   mRowMajorGridNormals       {};
    AssetPathString                mHeightmapTexturePath      {};
    AssetPathString                mGroundTexturePath         {};
    AssetPathString                mNoiseTexturePath          {};
    int                            mTestHeightmapWidth        {};
    int                            mTestHeightmapHeight       {};
    float                          mPower                     { 1 };
    Vector< unsigned char >        mTestHeightmapImageMemory  {};
    m4                             mWorldLevelEditorTransform {};


    // do you think this shit should actually be owned by the game presentation?
    Render::BufferHandle           mVertexBuffer              {};
    Render::BufferHandle           mIndexBuffer               {};
    int                            mIndexCount                {};
    Vector< TerrainOBB >           mTerrainOBBs               {};
    Errors                         mTestHeightmapLoadErrors   {};
  };

  TAC_META_DECL( Terrain );

#if 0
  struct CollideResult
  {
    bool mCollided {};
  };

  //CollideResult Collide( const Heightmap* heightmap, const Collider* collider );
#endif

}

