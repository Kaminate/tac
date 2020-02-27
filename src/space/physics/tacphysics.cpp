#include "space/collider/taccollider.h"
#include "space/graphics/tacgraphics.h"
#include "space/graphics/tacgraphics.h"
#include "space/physics/tacphysics.h"
#include "space/taccomponent.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/terrain/tacterrain.h"
#include "space/tacgjk.h"

#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/containers/tacVector.h"
#include "common/graphics/tacDebug3D.h"
#include "common/math/tacMath.h"
#include "common/tacMemory.h"
#include "common/tacPreprocessor.h"
#include "common/tacShell.h"
#include "common/thirdparty/stb_image.h"
#include "common/profile/tacProfile.h"

#include <array>
#include <algorithm>
#include <iostream>

extern void RegisterMetaphysics();


//static void DebugDrawGJK( const TacGJK& gjk, TacGraphics* graphics )
//{
//  if( !gjk.mIsColliding )
//  {
//    for( int iSupport = 0; iSupport < gjk.mSupports.size(); ++iSupport )
//    {
//      auto supporti = gjk.mSupports[ iSupport ];
//      graphics->DebugDrawSphere( supporti.mDiffPt, 0.1f );
//      for( int jSupport = iSupport + 1; jSupport < gjk.mSupports.size(); ++jSupport )
//      {
//        auto supportj = gjk.mSupports[ jSupport ];
//        graphics->DebugDrawLine( supporti.mDiffPt, supportj.mDiffPt );
//      }
//    }
//    return;
//  }
//
//  for( auto epaTri : gjk.mEPATriangles )
//  {
//    graphics->DebugDrawTriangle(
//      epaTri.mV0.mDiffPt,
//      epaTri.mV1.mDiffPt,
//      epaTri.mV2.mDiffPt );
//    graphics->DebugDrawSphere( epaTri.mV0.mDiffPt, 0.1f );
//    graphics->DebugDrawSphere( epaTri.mV1.mDiffPt, 0.1f );
//    graphics->DebugDrawSphere( epaTri.mV2.mDiffPt, 0.1f );
//  }
//  graphics->DebugDrawSphere( gjk.mEPAClosestSupportPoint.mDiffPt, 0.15f, v3( 1, 1, 0 ) );
//  graphics->DebugDrawArrow( { 0, 0, 0 }, gjk.mEPAClosest.mNormal * TacMax( gjk.mEPAClosest.mPlaneDist, 0.1f ), v3( 1, 1, 0 ) );
//
//}

TacPhysics::TacPhysics()
{
  RegisterMetaphysics();
  mShouldDebugDrawCapsules = true;
  mDebugDrawCapsuleColor = v3( 19, 122, 152 ) / 255.0f;
  mDebugDrawTerrainColor = v3( 122, 19, 152 ) / 255.0f;
  mGravity = -9.8f;
  mGravityOn = true;
  mRunning = true;
  mShouldDebugDrawTerrains = true;
  mShouldIntegrate = true;
  mShouldNarrowphase = true;
  mGJKDebugging = false;
  mDebugDrawCollision = true;
  mGJKDebugMaxIter = 10;
  mGJKDebugMaxEPAIter = 10;
}

TacCollider* TacPhysics::CreateCollider()
{
  auto collider = new TacCollider();
  mColliders.insert( collider );
  return collider;
}
TacTerrain* TacPhysics::CreateTerrain()
{
  auto terrain = new TacTerrain();
  mTerrains.insert( terrain );
  return terrain;
}
void TacPhysics::DestroyCollider( TacCollider* collider )
{
  mColliders.erase( collider );
  delete collider;
}
void TacPhysics::DestroyTerrain( TacTerrain* terrain )
{
  mTerrains.erase( terrain );
  delete terrain;
}


void TacPhysics::DebugImgui()
{


  //if( !ImGui::CollapsingHeader( "Physics" ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //ImGui::Checkbox( "Should debug draw capsules", &mShouldDebugDrawCapsules );
  //{
  //  ImGui::Indent();
  //  OnDestruct( ImGui::Unindent() );
  //  ImGui::ColorEdit3( "Capsule color", &mDebugDrawCapsuleColor[ 0 ] );
  //}
  //ImGui::Checkbox( "Running", &mRunning );
  //ImGui::Checkbox( "Should debug draw terrains", &mShouldDebugDrawTerrains );
  //if( ImGui::CollapsingHeader( "Collision Detection" ) )
  //{
  //  ImGui::Indent();
  //  OnDestruct( ImGui::Unindent() );
  //  ImGui::Checkbox( "Should Integrate", &mShouldIntegrate );
  //  ImGui::Checkbox( "Gravity On", &mGravityOn );
  //  ImGui::DragFloat( "Gravity", &mGravity, 0.01f );
  //  ImGui::Checkbox( "Should Narrowphase", &mShouldNarrowphase );
  //  ImGui::Checkbox( "Draw Collision", &mDebugDrawCollision );

  //  if( ImGui::CollapsingHeader( "GJK" ) )
  //  {
  //    ImGui::Indent();
  //    OnDestruct( ImGui::Unindent() );
  //    ImGui::Checkbox( "Debugging", &mGJKDebugging );
  //    ImGui::InputInt( "Max GJK Iter", &mGJKDebugMaxIter );
  //    ImGui::InputInt( "Max EPA Iter", &mGJKDebugMaxEPAIter );
  //    if( mGJKDebugMaxIter < 0 )
  //      mGJKDebugMaxIter = 0;
  //    if( mGJKDebugMaxEPAIter < 0 )
  //      mGJKDebugMaxEPAIter = 0;
  //  }
  //}

  //if( ImGui::CollapsingHeader( "Terrain" ) )
  //{
  //  ImGui::Indent();
  //  OnDestruct( ImGui::Unindent() );
  //  int iTerrain = 0;
  //  for( auto terrain : mTerrains )
  //  {
  //    ImGui::PushID( terrain );
  //    OnDestruct( ImGui::PopID() );
  //    if( !ImGui::CollapsingHeader( va( "Terrain %i", iTerrain++ ) ) )
  //      continue;
  //    if( ImGui::Button( "Add OBB" ) )
  //    {
  //      TacTerrainOBB obb = {};
  //      terrain->mTerrainOBBs.push_back( obb );
  //    }
  //    ImGui::Indent();
  //    OnDestruct( ImGui::Unindent() );
  //    for( int iOBB = 0; iOBB < ( int )terrain->mTerrainOBBs.size(); ++iOBB )
  //    {
  //      TacTerrainOBB& obb = terrain->mTerrainOBBs[ iOBB ];
  //      ImGui::PushID( &obb );
  //      OnDestruct( ImGui::PopID() );
  //      if( !ImGui::CollapsingHeader( ( va( "OBB %i", iOBB ) ) ) )
  //        continue;
  //      ImGui::Indent();
  //      OnDestruct( ImGui::Unindent() );
  //      ImGui::DragFloat3( "pos", obb.mPos.data(), 0.1f );
  //      ImGui::DragFloat3( "extents", obb.mHalfExtents.data(), 0.1f );
  //      ImGui::DragFloat3( "mEulerRads", obb.mEulerRads.data(), 0.1f );
  //    }
  //  }
  //}
}
void TacPhysics::DebugDrawCapsules()
{
  TacGraphics* graphics = TacGraphics::GetSystem( mWorld );
  for( auto collider : mColliders )
  {
    auto entity = collider->mEntity;
    v3 up( 0, 1, 0 );
    auto p0 = collider->mEntity->mRelativeSpace.mPosition + up * collider->mRadius;
    auto p1 = collider->mEntity->mRelativeSpace.mPosition + up * ( collider->mTotalHeight - collider->mRadius );
    //graphics->DebugDrawCapsule( p0, p1, collider->mRadius, mDebugDrawCapsuleColor );
  }
}
void TacPhysics::DebugDrawTerrains()
{
  TacGraphics* graphics = TacGraphics::GetSystem( mWorld );

  // Load heightmap mesh from heighap image
  for( auto terrain : mTerrains )
  {
    for( auto obb : terrain->mTerrainOBBs )
    {
      mWorld->mDebug3DDrawData->DebugDrawOBB(
        obb.mPos,
        obb.mHalfExtents,
        obb.mEulerRads,
        mDebugDrawTerrainColor );
    }
    // loads the heightmap from file into bitmap
    terrain->LoadTestHeightmap();

    terrain->PopulateGrid();
  }
}
void TacPhysics::Update()
{
  /*TAC_PROFILE_BLOCK*/;
  if( mShouldDebugDrawCapsules )
    DebugDrawCapsules();
  if( mShouldDebugDrawTerrains )
    DebugDrawTerrains();
  float dt = 1.0f / 60.0f; // TODO
  if( mShouldIntegrate )
    Integrate();
  if( mShouldNarrowphase )
    Narrowphase();
}
void TacPhysics::Integrate()
{
  /*TAC_PROFILE_BLOCK*/;
  v3 a( 0, mGravity, 0 );
  const auto dt = 1.0f / 60.0f;
  for( auto collider : mColliders )
  {
    if( mGravityOn )
      collider->mVelocity += a * dt;
    collider->mEntity->mRelativeSpace.mPosition += collider->mVelocity * dt;
  }
}
void TacPhysics::Narrowphase()
{
  /*TAC_PROFILE_BLOCK*/;
  TacGraphics* graphics = TacGraphics::GetSystem( mWorld );
  for( auto terrain : mTerrains )
  {
    for( auto obb : terrain->mTerrainOBBs )
    {
      auto terrainSupport = TacConvexPolygonSupport( obb.mPos, obb.mHalfExtents, obb.mEulerRads );

      for( auto collider : mColliders )
      {
        TacCapsuleSupport capsuleSupport;
        v3 up = { 0, 1, 0 };
        capsuleSupport.mRadius = collider->mRadius;
        capsuleSupport.mBotSpherePos =
          collider->mEntity->mRelativeSpace.mPosition
          + up * collider->mRadius;
        capsuleSupport.mTopSpherePos
          = collider->mEntity->mRelativeSpace.mPosition
          + up * ( collider->mTotalHeight - collider->mRadius );
        TacGJK gjk( &terrainSupport, &capsuleSupport );
        //OnDestruct( if( mGJKDebugging ) DebugDrawGJK( gjk, graphics ) );
        while( gjk.mIsRunning )
        {
          if( mGJKDebugging && gjk.mIteration >= mGJKDebugMaxIter )
            break;
          gjk.Step();
        }
        if( !gjk.mIsColliding )
          continue;
        while( !gjk.mEPAIsComplete )
        {
          if( mGJKDebugging && gjk.mEPAIteration >= mGJKDebugMaxEPAIter )
            break;
          gjk.EPAStep();
        }
        if( gjk.mEPABarycentricFucked )
          continue;
        collider->mEntity->mRelativeSpace.mPosition += gjk.mEPALeftNormal * gjk.mEPAPenetrationDist;
        auto projectedVel = TacProject( gjk.mEPALeftNormal, collider->mVelocity );
        if( TacDot( projectedVel, gjk.mEPALeftNormal ) < 0.0f )
        {
          collider->mVelocity -= projectedVel;
        }

        if( mDebugDrawCollision )
        {
          //graphics->DebugDrawSphere( gjk.mEPALeftPoint, 0.1f );
          //graphics->DebugDrawArrow(
          //  gjk.mEPALeftPoint,
          //  gjk.mEPALeftPoint + gjk.mEPALeftNormal * gjk.mEPAPenetrationDist );
        }

      }
    }
  }
  //TacVector< TacTerrainData* > terrainDatas;
  //for( auto terrain : mTerrains )
  //{
  //  auto terrainData = mGameInterface->mTerrainManager->GetAsset( terrain->mTerrainUUID );
  //  // don't do shit until all terrain is loaded?
  //  if( !terrainData )
  //  {
  //    allTerrainLoaded = false;
  //    continue;
  //  }
  //  terrainDatas.push_back( terrainData );
  //}
  //if( !mRunning || !allTerrainLoaded )
  //  return;
  /*
  // capsule - terrain narrowphase collision
  for( auto collider : mColliders )
  {
    auto colliderStuff = ( TacStuff* )collider->mEntity->GetComponent( TacComponentRegistryEntryIndex::Stuff );

    TacCapsuleSupport capsuleSupport( colliderStuff->mPosition, collider->mCapsuleHeight, collider->mCapsuleRadius );

    for( auto terrainData : terrainDatas )
    {
      //auto terrainStuff = ( TacStuff* )collider->mEntity->GetComponent( TacComponentRegistryEntryIndex::Stuff );
      TacUnusedParameter( colliderStuff );
      TacUnusedParameter( terrainData );


      for( auto& obb : terrainData->mOBBs )
      {
        TacConvexPolygonSupport convexPolygonSupport( obb );
        TacGJK gjk( &capsuleSupport, &convexPolygonSupport );
      }
    }
  }
  */
}

TacPhysics* TacPhysics::GetSystem( TacWorld* world )
{
  return ( TacPhysics* )world->GetSystem( TacPhysics::PhysicsSystemRegistryEntry );
}
TacCollideResult TacCollide( const TacHeightmap* heightmap, const TacCollider* collider )
{
  // get all overlapping triangles
  // get the one with the deepest penetration

  TacCollideResult result;
  return result;
}


TacSystemRegistryEntry* TacPhysics::PhysicsSystemRegistryEntry;

static TacSystem* TacCreatePhysicsSystem() { return new TacPhysics; }


void TacPhysicsDebugImgui( TacSystem* );
void TacPhysics::TacSpaceInitPhysics()
{
  TacPhysics::PhysicsSystemRegistryEntry = TacSystemRegistry::Instance()->RegisterNewEntry();
  TacPhysics::PhysicsSystemRegistryEntry->mCreateFn = TacCreatePhysicsSystem;
  TacPhysics::PhysicsSystemRegistryEntry->mName = "Physics";
  TacPhysics::PhysicsSystemRegistryEntry->mDebugImGui = TacPhysicsDebugImgui;
  TacTerrain::TacSpaceInitPhysicsTerrain();
  TacCollider::TacSpaceInitPhysicsCollider();
}
