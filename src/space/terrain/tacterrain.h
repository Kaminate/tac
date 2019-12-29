#pragma once
#include "common/containers/tacVector.h"
#include "common/math/tacVector3.h"
#include "common/math/tacMatrix4.h"
#include "common/tacErrorHandling.h"
#include "space/taccomponent.h"

struct TacTerrainOBB
{
  v3 mPos;
  v3 mHalfExtents;
  v3 mEulerRads;
};

struct TacHeightmap
{
  float mMaxHeight = 100.0f;
};

struct TacVertexBuffer;
struct TacIndexBuffer;

struct TacTerrain : public TacComponent
{
  static void TacSpaceInitPhysicsTerrain();

  TacComponentRegistryEntry* GetEntry() override;
  static TacComponentRegistryEntry* TerrainComponentRegistryEntry;
  static TacTerrain* GetComponent( TacEntity* );
  void LoadTestHeightmap();
  void PopulateGrid();
  v3 GetGridVal( int iRow, int iCol );
  void Recompute();

  int mSideVertexCount = 50;
  float mSideLength = 50.0f;
  float mUpwardsHeight = 20.0f;
  TacVector< v3 > mRowMajorGrid;
  TacString mHeightmapTexturePath = "assets/heightmap.png";
  TacString mGroundTexturePath = "";
  TacString mNoiseTexturePath = "";
  int mTestHeightmapWidth;
  int mTestHeightmapHeight;
  float mPower = 1;
  TacVector< uint8_t > mTestHeightmapImageMemory;
  m4 mWorldCreationTransform = {};
  TacVertexBuffer* mVertexBuffer = nullptr;
  TacIndexBuffer* mIndexBuffer = nullptr;
  TacVector< TacTerrainOBB > mTerrainOBBs;
  TacErrors mTestHeightmapLoadErrors;
};

extern int asdfDEBUG;

struct TacCollideResult
{
  bool mCollided = false;

};

//TacCollideResult TacCollide( const TacHeightmap* heightmap, const TacCollider* collider );
