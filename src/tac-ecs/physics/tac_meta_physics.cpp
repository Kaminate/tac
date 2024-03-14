#include "tac-ecs/physics/tac_physics.h"
#include "tac-std-lib/meta/tac_meta.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
namespace Tac
{

  void RegisterMetaphysics()
  {
    static bool registered = false;
    if( registered )
      return;
    registered = true;

    //auto meta = Meta::GetInstance();
    //auto metav3 = meta->GetType( Stringify( v3 ) );

    //Meta::Var mEulerRads;
    //mEulerRads.name = "mEulerRads";
    //mEulerRads.offset = OffsetOf( TerrainOBB, mEulerRads );
    //mEulerRads.mMetaType = metav3;

    //Meta::Var mHalfExtents;
    //mHalfExtents.name = "mHalfExtents";
    //mHalfExtents.offset = OffsetOf( TerrainOBB, mHalfExtents );
    //mHalfExtents.mMetaType = metav3;

    //Meta::Var mPos;
    //mPos.name = "mPos";
    //mPos.offset = OffsetOf( TerrainOBB, mPos );
    //mPos.mMetaType = metav3;

    //auto metaTerrainOBB = new Meta::Type();
    //metaTerrainOBB->name = Stringify( TerrainOBB );
    //metaTerrainOBB->mMetaVars = { mEulerRads, mPos, mHalfExtents };
    //metaTerrainOBB->size = sizeof( TerrainOBB );
    //meta->AddType( metaTerrainOBB );

    //Meta::Var obbs;
    //obbs.name = "mTerrainOBBs";
    //obbs.offset = OffsetOf( Terrain, mTerrainOBBs );
    //obbs.mMetaType = metaTerrainOBB;
    //obbs.mIsStdArray = true;
    //obbs.mDataFunction = []( void* data ) {
    //  return ( ( Vector< TerrainOBB >* )data )->data(); };
    //obbs.mResizeFunction = []( void* data, int size ) {
    //  ( ( Vector< TerrainOBB >* )data )->resize( size ); };

    //auto metaTerrain = new Meta::Type();
    //metaTerrain->name = Stringify( Terrain );
    //metaTerrain->mMetaVars = { obbs };
    //metaTerrain->size = sizeof( Terrain );
    //meta->AddType( metaTerrain );


  }

}

