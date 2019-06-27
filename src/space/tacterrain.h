#pragma once
#include "common/containers/tacVector.h"
#include "common/math/tacVector3.h"
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

  TacComponentRegistryEntry* GetEntry() override;
  static TacComponentRegistryEntry* ComponentRegistryEntry;

  static TacTerrain* GetComponent( TacEntity* );

  // heightmap
  // vertexes
  //int mRowCount = 2;
  //int mColumnCount = 2;
  //TacVector< float > mSamples;
  //TacVector< v3 > mModelSpaceGrid;
  //TacVector< v3 > mWorldSpaceGrid;
  //TacHeightmap mHeightmap;

  TacVector< v3 > mGrid;
};


struct TacCollideResult
{
  bool mCollided = false;

};

//TacCollideResult TacCollide( const TacHeightmap* heightmap, const TacCollider* collider );
