#include "tacphysics.h"
#include "common/tacmeta.h"
#include "common/tacPreprocessor.h"

void RegisterMetaphysics()
{
  static bool registered = false;
  if( registered )
    return;
  registered = true;
  auto meta = TacMeta::GetInstance();
  auto metav3 = meta->GetType( TacStringify( v3 ) );

  TacMeta::Var mEulerRads;
  mEulerRads.name = "mEulerRads";
  mEulerRads.offset = TacOffsetOf( TacTerrainOBB, mEulerRads );
  mEulerRads.mMetaType = metav3;

  TacMeta::Var mHalfExtents;
  mHalfExtents.name = "mHalfExtents";
  mHalfExtents.offset = TacOffsetOf( TacTerrainOBB, mHalfExtents );
  mHalfExtents.mMetaType = metav3;

  TacMeta::Var mPos;
  mPos.name = "mPos";
  mPos.offset = TacOffsetOf( TacTerrainOBB, mPos );
  mPos.mMetaType = metav3;

  auto metaTerrainOBB = new TacMeta::Type();
  metaTerrainOBB->name = TacStringify( TacTerrainOBB );
  metaTerrainOBB->mMetaVars = { mEulerRads, mPos, mHalfExtents };
  metaTerrainOBB->size = sizeof( TacTerrainOBB );
  meta->AddType( metaTerrainOBB );

  TacMeta::Var obbs;
  obbs.name = "mTerrainOBBs";
  obbs.offset = TacOffsetOf( TacTerrain, mTerrainOBBs );
  obbs.mMetaType = metaTerrainOBB;
  obbs.mIsStdArray = true;
  obbs.mDataFunction = []( void* data )
  {
    auto vec = ( TacVector< TacTerrainOBB >* )data;
    auto result = vec->data();
    return result;
  };
  obbs.mResizeFunction = []( void* data, int size )
  {
    auto vec = ( TacVector< TacTerrainOBB >* )data;
    vec->resize( size );
  };

  auto metaTerrain = new TacMeta::Type();
  metaTerrain->name = TacStringify( TacTerrain );
  metaTerrain->mMetaVars = { obbs };
  metaTerrain->size = sizeof( TacTerrain );
  meta->AddType( metaTerrain );


}