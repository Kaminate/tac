#include "src/space/physics/tac_physics.h" // self-inc

#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/math/tac_math.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/thirdparty/stb_image.h"
#include "src/space/collider/tac_collider.h"
#include "src/space/graphics/tac_graphics.h"
#include "src/space/tac_component.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_gjk.h"
#include "src/space/tac_world.h"
#include "src/space/terrain/tac_terrain.h"

#include <array>
#include <algorithm>

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  // Extern declarations

  extern void RegisterMetaphysics();

  extern void PhysicsDebugImgui( System* );

  // -----------------------------------------------------------------------------------------------

  // Static variables

  SystemRegistryEntry* Physics::PhysicsSystemRegistryEntry = nullptr;

  // -----------------------------------------------------------------------------------------------

  // Static functions

  static CapsuleSupport GetSupport( const Collider* collider )
  {
          const CapsuleSupport capsuleSupport( collider->mEntity->mRelativeSpace.mPosition,
                                               collider->mTotalHeight,
                                               collider->mRadius );
          return capsuleSupport;
  }

  static System* CreatePhysicsSystem() { return TAC_NEW Physics; }

#if TAC_TEMPORARILY_DISABLED()
  static void DebugDrawGJK( const GJK& gjk, Graphics* graphics )
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
    graphics->DebugDrawArrow( { 0, 0, 0 }, gjk.mEPAClosest.mNormal * Max( gjk.mEPAClosest.mPlaneDist, 0.1f ), v3( 1, 1, 0 ) );
  
  }
#endif

  // -----------------------------------------------------------------------------------------------

  Physics::Physics()
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

  Collider* Physics::CreateCollider()
  {
    auto collider = TAC_NEW Collider;
    mColliders.insert( collider );
    return collider;
  }

  Terrain*  Physics::CreateTerrain()
  {
    auto terrain = TAC_NEW Terrain;
    mTerrains.insert( terrain );
    return terrain;
  }

  void      Physics::DestroyCollider( Collider* collider )
  {
    mColliders.erase( collider );
    delete collider;
  }

  void      Physics::DestroyTerrain( Terrain* terrain )
  {
    mTerrains.erase( terrain );
    delete terrain;
  }


  void      Physics::DebugDrawCapsules()
  {
    //Graphics* graphics = GetGraphics( mWorld );
    for( Collider* collider : mColliders )
    {
      const CapsuleSupport capsuleSupport = GetSupport( collider );
      //graphics->DebugDrawCapsule( capsuleSupport.mBotSpherePos,
      //                               capsuleSupport.mTopSpherePos,
      //                               capsuleSupport.mRadius,
      //                               mDebugDrawCapsuleColor );
    }
  }
  void      Physics::DebugDrawTerrains()
  {
    //Graphics* graphics = GetGraphics( mWorld );

    // Load heightmap mesh from heighap image
    for( Terrain* terrain : mTerrains )
    {
      for( const TerrainOBB& obb : terrain->mTerrainOBBs )
      {
        mWorld->mDebug3DDrawData->DebugDraw3DOBB( obb.mPos,
                                                  obb.mHalfExtents,
                                                  obb.mEulerRads,
                                                  mDebugDrawTerrainColor );
      }
      // loads the heightmap from file into bitmap
      terrain->LoadTestHeightmap();

      terrain->PopulateGrid();
    }
  }

  void      Physics::Integrate()
  {
    /*TAC_PROFILE_BLOCK*/;
    const v3 a( 0, mGravity, 0 );
    const auto dt = 1.0f / 60.0f;
    for( Collider* collider : mColliders )
    {
      if( mGravityOn )
        collider->mVelocity += a * dt;

      collider->mEntity->mRelativeSpace.mPosition += collider->mVelocity * dt;
    }
  }

  void      Physics::Narrowphase()
  {
    /*TAC_PROFILE_BLOCK*/;
    //Graphics* graphics = GetGraphics( mWorld );
    for( Terrain* terrain : mTerrains )
    {
      for( const TerrainOBB& obb : terrain->mTerrainOBBs )
      {
        const ConvexPolygonSupport terrainSupport( obb.mPos, obb.mHalfExtents, obb.mEulerRads );
        for( Collider* collider : mColliders )
        {
          const CapsuleSupport capsuleSupport = GetSupport( collider );

          GJK gjk( &terrainSupport, &capsuleSupport );

#if TAC_TEMPORARILY_DISABLED()
          OnDestruct( if( mGJKDebugging ) DebugDrawGJK( gjk, graphics ) );
#endif

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
          const v3 projectedVel = Project( gjk.mEPALeftNormal, collider->mVelocity );
          if( Dot( projectedVel, gjk.mEPALeftNormal ) < 0.0f )
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
    //Vector< TerrainData* > terrainDatas;
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
      auto colliderStuff = ( Stuff* )collider->mEntity->GetComponent( ComponentRegistryEntryIndex::Stuff );

      CapsuleSupport capsuleSupport( colliderStuff->mPosition, collider->mCapsuleHeight, collider->mCapsuleRadius );

      for( auto terrainData : terrainDatas )
      {
        //auto terrainStuff = ( Stuff* )collider->mEntity->GetComponent( ComponentRegistryEntryIndex::Stuff );
        UnusedParameter( colliderStuff );
        UnusedParameter( terrainData );


        for( auto& obb : terrainData->mOBBs )
        {
          ConvexPolygonSupport convexPolygonSupport( obb );
          GJK gjk( &capsuleSupport, &convexPolygonSupport );
        }
      }
    }
    */
  }

  // -----------------------------------------------------------------------------------------------

  // override functions

  void      Physics::Update()
  {
    /*TAC_PROFILE_BLOCK*/;
    if( mShouldDebugDrawCapsules )
      DebugDrawCapsules();

    if( mShouldDebugDrawTerrains )
      DebugDrawTerrains();

    if( mShouldIntegrate )
      Integrate();

    if( mShouldNarrowphase )
      Narrowphase();
  }

  void      Physics::DebugImgui()
  {
#if TAC_TEMPORARILY_DISABLED()

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
          TerrainOBB obb = {};
          terrain->mTerrainOBBs.push_back( obb );
        }
        ImGui::Indent();
        OnDestruct( ImGui::Unindent() );
        for( int iOBB = 0; iOBB < ( int )terrain->mTerrainOBBs.size(); ++iOBB )
        {
          TerrainOBB& obb = terrain->mTerrainOBBs[ iOBB ];
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
#endif
  }

  // -----------------------------------------------------------------------------------------------

  // static functions

  void Physics::SpaceInitPhysics()
  {
    Physics::PhysicsSystemRegistryEntry = SystemRegisterNewEntry();
    Physics::PhysicsSystemRegistryEntry->mCreateFn = CreatePhysicsSystem;
    Physics::PhysicsSystemRegistryEntry->mName = "Physics";
    Physics::PhysicsSystemRegistryEntry->mDebugImGui = PhysicsDebugImgui;
    Terrain::SpaceInitPhysicsTerrain();
    RegisterColliderComponent();
  }

  Physics* Physics::GetSystem( World* world )
  {
    return ( Physics* )world->GetSystem( Physics::PhysicsSystemRegistryEntry );
  }


  // -----------------------------------------------------------------------------------------------

} // namespace Tac

