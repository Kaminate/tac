#include "tac_world.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/physics/collider/tac_collider.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/player/tac_player.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"

namespace Tac
{
  static void ComputeTransformsRecursivelyAux( const m4& parentTransform, Entity* entity )
  {
    const m4 localTransform{ m4::Transform( entity->mRelativeSpace.mScale,
                                       entity->mRelativeSpace.mEulerRads,
                                       entity->mRelativeSpace.mPosition ) };
    const m4 worldTransform { parentTransform * localTransform };
    const m4 localTransformNoScale{ m4::Transform( v3( 1, 1, 1 ),
                                              entity->mRelativeSpace.mEulerRads,
                                              entity->mRelativeSpace.mPosition ) };
    const m4 worldTransformNoScale { parentTransform * localTransformNoScale };
    entity->mWorldTransform = worldTransform;
    entity->mWorldPosition = ( worldTransform * v4( 0, 0, 0, 1 ) ).xyz();
    for( Entity* child : entity->mChildren )
    {
      const m4& parentTransformForChild{ child->mInheritParentScale
        ? worldTransformNoScale
        : worldTransform };
      ComputeTransformsRecursivelyAux( parentTransformForChild, child );
    }
  }

  World::World()
  {
    Init();
  }

  World::~World()
  {
    for( Player* player : mPlayers )
      TAC_DELETE player;

    for( Entity* entity : mEntities )
      TAC_DELETE entity;

    // Delete system last, so entities can remove their components
    for( System* system : mSystems )
      TAC_DELETE system;

    TAC_DELETE mDebug3DDrawData ;
  }

  void World::Init()
  {
    if( mSystems.empty() )
    {
      for( const SystemInfo& entry : SystemInfo::Iterate() )
      {
        System* system{ entry.mCreateFn() };
        TAC_ASSERT( system );
        system->mWorld = this;
        mSystems.push_back( system );
      }
      mDebug3DDrawData = TAC_NEW Debug3DDrawData;
    }
  }

  auto World::SpawnEntity( EntityUUID entityUUID ) -> Entity*
  {
    TAC_ASSERT( entityUUID != NullEntityUUID );
    Entity* entity { TAC_NEW Entity };
    entity->mEntityUUID = entityUUID;
    entity->mWorld = this;
    mEntities.push_front( entity );
    return entity;
  }

  auto World::FindEntity( PlayerUUID playerUUID ) -> Entity*
  {
    Player* player { FindPlayer( playerUUID ) };
    return player ? FindEntity( player->mEntityUUID ) : nullptr;
  }

  auto World::FindEntity( EntityUUID entityUUID ) -> Entity*
  {
    for( Entity* entity : mEntities )
      if( entity->mEntityUUID == entityUUID )
        return entity;
    return nullptr;
  }

  auto World::FindEntity( StringView name ) -> Entity*
  {
    for( Entity* entity : mEntities )
      if( ( StringView )entity->mName == name )
        return entity;
    return nullptr;
  }

  void World::KillEntity( EntityIterator it )
  {
    TAC_ASSERT( it != mEntities.end() );
    Entity* entity { *it };

    // Remove this entity from its parent's list of children
    if( Entity * parent{ entity->mParent } )
    {
      int iEntity {};
      int entityCount { parent->mChildren.size() };
      while( iEntity < entityCount )
      {
        if( parent->mChildren[ iEntity ] == entity )
        {
          break;
        }
        iEntity++;
      }
      TAC_ASSERT( iEntity < entityCount );
      parent->mChildren[ iEntity ] = parent->mChildren[ entityCount - 1 ];
      parent->mChildren.pop_back();
    }

    Vector< EntityIterator > treeIterators  { it };
    int iTreeIterator {};
    for( ;; )
    {
      if( iTreeIterator == treeIterators.size() )
        break;

      EntityIterator treeIterator { treeIterators[ iTreeIterator ] };
      Entity* treeEntity { *treeIterator };
      for( Entity* treeEntityChild : treeEntity->mChildren )
      {
        EntityIterator treeEntityChildIterator { mEntities.Find( treeEntityChild ) };
        treeIterators.push_back( treeEntityChildIterator );
      }
      iTreeIterator++;
    }

    for( EntityIterator treeIterator : treeIterators )
    {
      Entity* treeEntity { *treeIterator };
      if( Player * player{ FindPlayer( treeEntity->mEntityUUID ) } )
        player->mEntityUUID = NullEntityUUID;

      mEntities.erase( treeIterator );
      TAC_DELETE treeEntity;
    }
  }

  void World::KillEntity( Entity* entity )
  {
    auto it { Find( mEntities.begin(), mEntities.end(), entity ) };
    KillEntity( it );
  }

  void World::KillEntity( EntityUUID entityUUID )
  {
    auto it{ FindIf( mEntities.begin(),
                      mEntities.end(),
                      [ & ]( Entity* entity ) { return entity->mEntityUUID == entityUUID; } ) };
    KillEntity( it );
  }

  auto World::SpawnPlayer( PlayerUUID playerUUID ) -> Player*
  {
    auto player { TAC_NEW Player() };
    player->mPlayerUUID = playerUUID;
    player->mWorld = this;
    mPlayers.push_front( player );
    return player;
  }

  auto World::FindPlayer( PlayerUUID playerUUID ) -> Player*
  {
    for( Player* player : mPlayers )
      if( player->mPlayerUUID == playerUUID )
        return player;
    return nullptr;
  }

  auto World::FindPlayer( EntityUUID entityUUID ) -> Player*
  {
    for( Player* player : mPlayers )
      if( player->mEntityUUID == entityUUID )
        return player;
    return nullptr;
  }

  void World::KillPlayer( PlayerUUID playerUUID )
  {
    auto it{ FindIf( mPlayers.begin(),
                      mPlayers.end(),
                      [ & ]( Player* player ) { return player->mPlayerUUID == playerUUID; } ) };
    TAC_ASSERT( it != mPlayers.end() );
    Player* player { *it };
    KillEntity( player->mEntityUUID );
    TAC_DELETE player;
    mPlayers.erase( it );
  }

  void World::ApplyInput( Player* player, float seconds )
  {
    TAC_UNUSED_PARAMETER( seconds );
    Entity* entity { FindEntity( player->mEntityUUID ) };
    if( !entity )
      return;

    // update velocity
    Collider* collider { Collider::GetCollider( entity ) };
    if( !collider )
      return;

    float speedHorizontal { 5 };
    float speedVertical { 7 };
    // temp, align camera with player movement shit
    float hack { -1 };
    float velX { player->mInputDirection.x * speedHorizontal };
    float velY { player->mIsSpaceJustDown ? speedVertical : collider->mVelocity.y };
    float velZ { player->mInputDirection.y * speedHorizontal * hack };
    collider->mVelocity = v3( velX, velY, velZ );

    // update rotation
    //auto stuff = ( Stuff* )entity->GetComponent( ComponentInfoIndex::Stuff );
    //stuff->mWaddleParams.Update( player->mInputDirection, seconds );
    //stuff->zCCWEulerRotDeg = stuff->mWaddleParams.mAngle;
  }

  void World::ComputeTransformsRecursively()
  {
    const m4 identity { m4::Identity() };
    for( Entity* entity : mEntities )
      if( !entity->mParent )
        ComputeTransformsRecursivelyAux( identity, entity );
  }

  void World::Step( float seconds )
  {
    TAC_PROFILE_BLOCK;

    ComputeTransformsRecursively();

    for( Player* player : mPlayers )
      ApplyInput( player, seconds );

    for( System* system : mSystems )
      system->Update();

    for( Entity* entity : mEntities )
    {
      TAC_UNUSED_PARAMETER( entity );
      //entity->Integrate( seconds );
    }


    mElapsedSecs += seconds;
#if TAC_TEMPORARILY_DISABLED()
    if( mDebugDrawEntityOrigins )
    {
      v3 boxSize { v3( 1, 1, 1 ) * 0.1f };
      v3 boxRot  {};
      Graphics* graphics { Graphics::From( this ) };
      if( graphics )
      {
        for( Entity* entity : mEntities )
        {
          TAC_UNUSED_PARAMETER( entity );
          graphics->DebugDrawOBB( entity->mPosition, boxSize, boxRot );
        }
      }
    }
#endif
  }

  void World::DeepCopy( const World& world )
  {
    for( Player* player : mPlayers )
      TAC_DELETE player;

    mPlayers.clear();

    for( Entity* entity : mEntities )
      TAC_DELETE entity;

    mEntities.clear();

    mElapsedSecs = world.mElapsedSecs;

    for( const Player* fromPlayer : world.mPlayers )
    {
      auto player { TAC_NEW Player };
      *player = *fromPlayer;
      mPlayers.push_front( player );
    }

    for( Entity* fromEntity : world.mEntities )
    {
      Entity* toEntity { SpawnEntity( fromEntity->mEntityUUID ) };
      toEntity->DeepCopy( *fromEntity );
    }

    mDebug3DDrawData->CopyFrom( *world.mDebug3DDrawData );
  }

  auto World::GetSystem( const SystemInfo* systemInfo ) dynmc -> dynmc System*
  {
    return mSystems[ systemInfo->GetIndex() ];
  }

  auto World::GetSystem( const SystemInfo* systemInfo ) const -> const System*
  {
    return mSystems[ systemInfo->GetIndex() ];
  }

  void World::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "World" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //if( ImGui::CollapsingHeader( "Entities" ) )
    //{
    //  ImGui::Indent();
    //  OnDestruct( ImGui::Unindent() );
    //  for( auto entity : mEntities )
    //    entity->DebugImgui();
    //}

    //if( ImGui::CollapsingHeader( "Players" ) )
    //{
    //  ImGui::Indent();
    //  OnDestruct( ImGui::Unindent() );
    //  for( auto player : mPlayers )
    //    player->DebugImgui();
    //}

    //for( auto system : mSystems )
    //  system->DebugImgui();

    //ImGui::Checkbox( "Draw Entity Origins", &mDebugDrawEntityOrigins );

  }

} // namespaceTac
