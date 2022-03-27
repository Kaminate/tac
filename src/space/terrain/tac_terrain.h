#pragma once
#include "src/common/containers/tacVector.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/space/tacComponent.h"

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
    String                         mHeightmapTexturePath;
    String                         mGroundTexturePath = "";
    String                         mNoiseTexturePath = "";
    int                            mTestHeightmapWidth = 0;
    int                            mTestHeightmapHeight = 0;
    float                          mPower = 1;
    Vector< unsigned char >        mTestHeightmapImageMemory;
    m4                             mWorldCreationTransform = {};


    // do you think this shit should actually be owned by the game presentation?
    Render::VertexBufferHandle     mVertexBuffer;
    Render::IndexBufferHandle      mIndexBuffer;
    int                            mIndexCount = 0;
    Vector< TerrainOBB >           mTerrainOBBs;
    Errors                         mTestHeightmapLoadErrors;
  };


  struct CollideResult
  {
    bool mCollided = false;

  };

  //CollideResult Collide( const Heightmap* heightmap, const Collider* collider );

}
