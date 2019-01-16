#include "tacphysics.h"
#include "taccomponent.h"
#include "tacgraphics.h"
#include "tacworld.h"
#include "taccollider.h"
#include "tacgraphics.h"
#include "tacentity.h"

#include "common/containers/tacVector.h"
#include "common/math/tacMath.h"
#include "common/tacPreprocessor.h"
#include "common/imgui.h"

#include <array>
#include <algorithm>
#include <iostream>

extern void RegisterMetaphysics();

static void DebugDrawGJK( const TacGJK& gjk, TacGraphics* graphics )
{
  if( !gjk.mIsColliding )
  {
    for( int iSupport = 0; iSupport < gjk.mSupports.size(); ++iSupport )
    {
      auto supporti = gjk.mSupports[ iSupport ];
      graphics->DebugDrawSphere( supporti.mDiffPt, 0.1f );
      for( int jSupport = iSupport + 1; jSupport < gjk.mSupports.size(); ++jSupport )
      {
        auto supportj = gjk.mSupports[ jSupport ];
        graphics->DebugDrawLine( supporti.mDiffPt, supportj.mDiffPt );
      }
    }
    return;
  }

  for( auto epaTri : gjk.mEPATriangles )
  {
    graphics->DebugDrawTriangle(
      epaTri.mV0.mDiffPt,
      epaTri.mV1.mDiffPt,
      epaTri.mV2.mDiffPt );
    graphics->DebugDrawSphere( epaTri.mV0.mDiffPt, 0.1f );
    graphics->DebugDrawSphere( epaTri.mV1.mDiffPt, 0.1f );
    graphics->DebugDrawSphere( epaTri.mV2.mDiffPt, 0.1f );
  }
  graphics->DebugDrawSphere( gjk.mEPAClosestSupportPoint.mDiffPt, 0.15f, v3( 1, 1, 0 ) );
  graphics->DebugDrawArrow( { 0, 0, 0 }, gjk.mEPAClosest.mNormal * TacMax( gjk.mEPAClosest.mPlaneDist, 0.1f ), v3( 1, 1, 0 ) );

}

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
TacComponent* TacPhysics::CreateComponent( TacComponentType componentType )
{
  switch( componentType )
  {
    case TacComponentType::Collider:
    {
      auto collider = new TacCollider();
      mColliders.insert( collider );
      return collider;
    }
    //case TacComponentType::Terrain:
    //{
    //  auto terrain = new TacTerrain();
    //  mTerrains.insert( terrain );
    //  return terrain;
    //}
  }
  TacInvalidCodePath;
  return nullptr;
}
void TacPhysics::DestroyComponent( TacComponent* component )
{
  auto componentType = component->GetComponentType();
  switch( componentType )
  {
    case TacComponentType::Collider:
    {
      auto collider = ( TacCollider* )component;
      auto it = mColliders.find( collider );
      TacAssert( it != mColliders.end() );
      mColliders.erase( it );
      delete collider;
    } break;

    //  case TacComponentType::Terrain:
    //  {
    //    auto terrain = ( TacTerrain* )component;
    //    auto it = mTerrains.find( terrain );
    //    TacAssert( it != mTerrains.end() );
    //    mTerrains.erase( it );
    //    delete terrain;
    //  } break;

    TacInvalidDefaultCase( componentType );
  }
}
void TacPhysics::DebugImgui()
{
  if( !ImGui::CollapsingHeader( "Physics" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  ImGui::Checkbox( "Should debug draw capsules", &mShouldDebugDrawCapsules );
  {
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    ImGui::ColorEdit3( "Capsule color", &mDebugDrawCapsuleColor[ 0 ] );
  }
  ImGui::Checkbox( "Running", &mRunning );
  ImGui::Checkbox( "Should debug draw terrains", &mShouldDebugDrawTerrains );
  if( ImGui::CollapsingHeader( "Collision Detection" ) )
  {
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    ImGui::Checkbox( "Should Integrate", &mShouldIntegrate );
    ImGui::Checkbox( "Gravity On", &mGravityOn );
    ImGui::DragFloat( "Gravity", &mGravity, 0.01f );
    ImGui::Checkbox( "Should Narrowphase", &mShouldNarrowphase );
    ImGui::Checkbox( "Draw Collision", &mDebugDrawCollision );

    if( ImGui::CollapsingHeader( "GJK" ) )
    {
      ImGui::Indent();
      OnDestruct( ImGui::Unindent() );
      ImGui::Checkbox( "Debugging", &mGJKDebugging );
      ImGui::InputInt( "Max GJK Iter", &mGJKDebugMaxIter );
      ImGui::InputInt( "Max EPA Iter", &mGJKDebugMaxEPAIter );
      if( mGJKDebugMaxIter < 0 )
        mGJKDebugMaxIter = 0;
      if( mGJKDebugMaxEPAIter < 0 )
        mGJKDebugMaxEPAIter = 0;
    }
  }

  if( ImGui::CollapsingHeader( "Terrain" ) )
  {
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    int iTerrain = 0;
    for( auto terrain : mTerrains )
    {
      ImGui::PushID( terrain );
      OnDestruct( ImGui::PopID() );
      if( !ImGui::CollapsingHeader( va( "Terrain %i", iTerrain++ ) ) )
        continue;
      if( ImGui::Button( "Add OBB" ) )
      {
        TacTerrainOBB obb = {};
        terrain->mTerrainOBBs.push_back( obb );
      }
      ImGui::Indent();
      OnDestruct( ImGui::Unindent() );
      for( int iOBB = 0; iOBB < ( int )terrain->mTerrainOBBs.size(); ++iOBB )
      {
        TacTerrainOBB& obb = terrain->mTerrainOBBs[ iOBB ];
        ImGui::PushID( &obb );
        OnDestruct( ImGui::PopID() );
        if( !ImGui::CollapsingHeader( ( va( "OBB %i", iOBB ) ) ) )
          continue;
        ImGui::Indent();
        OnDestruct( ImGui::Unindent() );
        ImGui::DragFloat3( "pos", obb.mPos.data(), 0.1f );
        ImGui::DragFloat3( "extents", obb.mHalfExtents.data(), 0.1f );
        ImGui::DragFloat3( "mEulerRads", obb.mEulerRads.data(), 0.1f );
      }
    }
  }
}
void TacPhysics::DebugDrawCapsules()
{
  auto graphics = ( TacGraphics* )mWorld->GetSystem( TacSystemType::Graphics );
  for( auto collider : mColliders )
  {
    auto entity = collider->mEntity;
    v3 up( 0, 1, 0 );
    auto p0 = collider->mEntity->mPosition + up * collider->mRadius;
    auto p1 = collider->mEntity->mPosition + up * ( collider->mTotalHeight - collider->mRadius );
    graphics->DebugDrawCapsule( p0, p1, collider->mRadius, mDebugDrawCapsuleColor );
  }
}
void TacPhysics::DebugDrawTerrains()
{
  auto graphics = ( TacGraphics* )mWorld->GetSystem( TacSystemType::Graphics );
  for( auto terrain : mTerrains )
  {
    for( auto obb : terrain->mTerrainOBBs )
    {
      graphics->DebugDrawOBB( obb.mPos, obb.mHalfExtents, obb.mEulerRads, mDebugDrawTerrainColor );
    }
  }
}
void TacPhysics::Update()
{
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
  v3 a( 0, mGravity, 0 );
  const auto dt = 1.0f / 60.0f;
  for( auto collider : mColliders )
  {
    if( mGravityOn )
      collider->mVelocity += a * dt;
    collider->mEntity->mPosition += collider->mVelocity * dt;
  }
}
void TacPhysics::Narrowphase()
{
  auto graphics = ( TacGraphics* )mWorld->GetSystem( TacSystemType::Graphics );
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
          collider->mEntity->mPosition
          + up * collider->mRadius;
        capsuleSupport.mTopSpherePos
          = collider->mEntity->mPosition
          + up * ( collider->mTotalHeight - collider->mRadius );
        TacGJK gjk( &terrainSupport, &capsuleSupport );
        OnDestruct( if( mGJKDebugging ) DebugDrawGJK( gjk, graphics ) );
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
        collider->mEntity->mPosition += gjk.mEPALeftNormal * gjk.mEPAPenetrationDist;
        auto projectedVel = TacProject( gjk.mEPALeftNormal, collider->mVelocity );
        if( TacDot( projectedVel, gjk.mEPALeftNormal ) < 0.0f )
        {
          collider->mVelocity -= projectedVel;
        }

        if( mDebugDrawCollision )
        {
          graphics->DebugDrawSphere( gjk.mEPALeftPoint, 0.1f );
          graphics->DebugDrawArrow(
            gjk.mEPALeftPoint,
            gjk.mEPALeftPoint + gjk.mEPALeftNormal * gjk.mEPAPenetrationDist );
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
    auto colliderStuff = ( TacStuff* )collider->mEntity->GetComponent( TacComponentType::Stuff );

    TacCapsuleSupport capsuleSupport( colliderStuff->mPosition, collider->mCapsuleHeight, collider->mCapsuleRadius );

    for( auto terrainData : terrainDatas )
    {
      //auto terrainStuff = ( TacStuff* )collider->mEntity->GetComponent( TacComponentType::Stuff );
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
