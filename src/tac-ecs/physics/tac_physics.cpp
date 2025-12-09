#include "tac_physics.h" // self-inc

#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/thirdparty/stb/stb_image.h"
#include "tac-ecs/physics/collider/tac_collider.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/physics/tac_gjk.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/terrain/tac_terrain.h"

namespace Tac
{
  // -----------------------------------------------------------------------------------------------

  // Extern declarations

  extern void RegisterMetaphysics();

  extern void PhysicsDebugImgui( System* );

  // -----------------------------------------------------------------------------------------------

  // Static variables

  SystemInfo* Physics::sInfo {};

  // -----------------------------------------------------------------------------------------------

  // Static functions

#if 0
  static auto GetSupport( const Collider* collider ) -> CapsuleSupport
  {
    return CapsuleSupport( collider->mEntity->mRelativeSpace.mPosition,
                                         collider->mTotalHeight,
                                         collider->mRadius );
  }
#endif

  static auto CreatePhysicsSystem() -> System* { return TAC_NEW Physics; }

#if TAC_TEMPORARILY_DISABLED()
  static void DebugDrawGJK( const GJK& gjk, Graphics* graphics )
  {
    if( !gjk.mIsColliding )
    {
      for( int iSupport {}; iSupport < gjk.mSupports.size(); ++iSupport )
      {
        auto supporti { gjk.mSupports[ iSupport ] };
        graphics->DebugDrawSphere( supporti.mDiffPt, 0.1f );
        for( int jSupport { iSupport + 1 }; jSupport < gjk.mSupports.size(); ++jSupport )
        {
          auto supportj { gjk.mSupports[ jSupport ] };
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
  }

  auto Physics::CreateCollider() -> Collider*
  {
    auto collider { TAC_NEW Collider };
    mColliders.insert( collider );
    return collider;
  }

  auto Physics::CreateTerrain() -> Terrain*
  {
    auto terrain { TAC_NEW Terrain };
    mTerrains.insert( terrain );
    return terrain;
  }

  void Physics::DestroyCollider( Collider* collider )
  {
    mColliders.erase( collider );
    TAC_DELETE collider;
  }

  void Physics::DestroyTerrain( Terrain* terrain )
  {
    mTerrains.erase( terrain );
    TAC_DELETE terrain;
  }


  void Physics::DebugDrawCapsules()
  {
    //Graphics* graphics = Graphics::Get( mWorld );
    for( Collider* collider : mColliders )
    {
      //const CapsuleSupport capsuleSupport { GetSupport( collider ) };
      //graphics->DebugDrawCapsule( capsuleSupport.mBotSpherePos,
      //                               capsuleSupport.mTopSpherePos,
      //                               capsuleSupport.mRadius,
      //                               mDebugDrawCapsuleColor );
    }
  }
  void Physics::DebugDrawTerrains()
  {
    //Graphics* graphics = Graphics::Get( mWorld );

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

  void Physics::Integrate()
  {
    /*TAC_PROFILE_BLOCK*/;
    const v3 a( 0, mGravity, 0 );
    const auto dt { 1.0f / 60.0f };
    for( Collider* collider : mColliders )
    {
      if( mGravityOn )
        collider->mVelocity += a * dt;

      collider->mEntity->mRelativeSpace.mPosition += collider->mVelocity * dt;
    }
  }

  void Physics::Narrowphase()
  {
    /*TAC_PROFILE_BLOCK*/;
    //Graphics* graphics = Graphics::Get( mWorld );
    for( Terrain* terrain : mTerrains )
    {
      for( const TerrainOBB& obb : terrain->mTerrainOBBs )
      {
        const ConvexPolygonSupport terrainSupport( obb.mPos, obb.mHalfExtents, obb.mEulerRads );
        for( Collider* collider : mColliders )
        {
          //const CapsuleSupport capsuleSupport { GetSupport( collider ) };

          //GJK gjk( &terrainSupport, &capsuleSupport );
          GJK gjk;

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
          const v3 projectedVel { Project( gjk.mEPALeftNormal, collider->mVelocity ) };
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
      auto colliderStuff = ( Stuff* )collider->mEntity->GetComponent( ComponentInfoIndex::Stuff );

      CapsuleSupport capsuleSupport( colliderStuff->mPosition, collider->mCapsuleHeight, collider->mCapsuleRadius );

      for( auto terrainData : terrainDatas )
      {
        //auto terrainStuff = ( Stuff* )collider->mEntity->GetComponent( ComponentInfoIndex::Stuff );
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

  void Physics::Update()
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

  void Physics::DebugImgui()
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
      int iTerrain {};
      for( Terrain* terrain : mTerrains )
      {
        ImGui::PushID( terrain );
        OnDestruct( ImGui::PopID() );
        if( !ImGui::CollapsingHeader( ShortFixedString::Concat( "Terrain ", ToString( iTerrain++ ) ) ) )
          continue;

        if( ImGui::Button( "Add OBB" ) )
        {
          TerrainOBB obb  {};
          terrain->mTerrainOBBs.push_back( obb );
        }
        ImGui::Indent();
        OnDestruct( ImGui::Unindent() );
        for( int iOBB {}; iOBB < ( int )terrain->mTerrainOBBs.size(); ++iOBB )
        {
          TerrainOBB& obb = terrain->mTerrainOBBs[ iOBB ];
          ImGui::PushID( &obb );
          OnDestruct( ImGui::PopID() );
          if( !ImGui::CollapsingHeader( ( ShortFixedString::Concat( "OBB ", ToString(iOBB) ) ) ) )
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
    sInfo = SystemInfo::Register();
    sInfo->mName = "Physics";
    sInfo->mCreateFn = CreatePhysicsSystem;
    sInfo->mDebugImGui = PhysicsDebugImgui;

    Terrain::SpaceInitPhysicsTerrain();
    Collider::RegisterComponent();
  }

  auto Physics::GetSystem( dynmc World* world ) -> dynmc Physics*
  {
    return ( dynmc Physics* )world->GetSystem( Physics::sInfo );
  }

  auto Physics::GetSystem( const World* world ) -> const Physics*
  {
    return ( const Physics* )world->GetSystem( Physics::sInfo );
  }


  // -----------------------------------------------------------------------------------------------

} // namespace Tac

