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


struct TacTerrain : public TacComponent
{
  TacVector< TacTerrainOBB > mTerrainOBBs;
  static void TacSpaceInitPhysicsTerrain();

  TacComponentRegistryEntry* GetEntry() override;
  static TacComponentRegistryEntry* TerrainComponentRegistryEntry;

  static TacTerrain* GetComponent( TacEntity* );
  void LoadTestHeightmap();
  void PopulateGrid();
  void DebugDraw();
  v3 GetVal( int x, int y );

  int mSideVertexCount = 50;

  float mSideLength = 50.0f;
  float mHeight = 20.0f;

  // heightmap
  // vertexes
  //int mRowCount = 2;
  //int mColumnCount = 2;
  //TacVector< float > mSamples;
  //TacVector< v3 > mModelSpaceGrid;
  //TacVector< v3 > mWorldSpaceGrid;
  //TacHeightmap mHeightmap;

  TacVector< v3 > mGrid;

  TacString mHeightmapTexturePath = "assets/heightmap.png";
  int mTestHeightmapWidth;
  int mTestHeightmapHeight;
  float mPower = 1;
  TacVector< uint8_t > mTestHeightmapImageMemory;
  TacErrors mTestHeightmapLoadErrors;
  m4 mWorldCreationTransform = {};
};

extern int asdfDEBUG;

struct TacCollideResult
{
  bool mCollided = false;

};

//TacCollideResult TacCollide( const TacHeightmap* heightmap, const TacCollider* collider );
