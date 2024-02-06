#include "space/world/tac_world.h" // self-inc

#include "space/ecs/tac_entity.h"
#include "space/ecs/tac_system.h"
#include "space/graphics/tac_graphics.h"
#include "space/physics/collider/tac_collider.h"
#include "space/physics/tac_physics.h"
#include "space/player/tac_player.h"
#include "src/common/graphics/tac_debug_3d.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/profile/tac_profile.h"

Tac::World::World()
{
  for( const SystemRegistryEntry& entry : SystemRegistryIterator() )
  {
    System* system = entry.mCreateFn();
    TAC_ASSERT( system );
    system->mWorld = this;
    mSystems.push_back( system );
  }
  mDebug3DDrawData = TAC_NEW Debug3DDrawData;
}

Tac::World::~World()
{
  for( System* system : mSystems )
    delete system;

  for( Player* player : mPlayers )
    delete player;

  for( Entity* entity : mEntities )
    delete entity;
}

Tac::Entity*       Tac::World::SpawnEntity( EntityUUID entityUUID )
{
  auto entity = TAC_NEW Entity();
  entity->mEntityUUID = entityUUID;
  entity->mWorld = this;
  mEntities.PushFront( entity );
  return entity;
}

Tac::Entity*       Tac::World::FindEntity( PlayerUUID playerUUID )
{
  Player* player = FindPlayer( playerUUID );
  return player ? FindEntity( player->mEntityUUID ) : nullptr;
}

Tac::Entity*       Tac::World::FindEntity( EntityUUID entityUUID )
{
  for( Entity* entity : mEntities )
    if( entity->mEntityUUID == entityUUID )
      return entity;
  return nullptr;
}

Tac::Entity*       Tac::World::FindEntity( StringView name )
{
  for( Entity* entity : mEntities )
    if( entity->mName == name )
      return entity;
  return nullptr;
}

void               Tac::World::KillEntity( EntityIterator it )
{
  TAC_ASSERT( it != mEntities.end() );
  Entity* entity = *it;

  // Remove this entity from its parent's list of children
  if( Entity* parent = entity->mParent )
  {
    int iEntity = 0;
    int entityCount = parent->mChildren.size();
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

  Vector< EntityIterator > treeIterators = { it };
  int iTreeIterator = 0;
  for( ;; )
  {
    if( iTreeIterator == treeIterators.size() )
      break;
    EntityIterator treeIterator = treeIterators[ iTreeIterator ];
    Entity* treeEntity = *treeIterator;
    for( Entity* treeEntityChild : treeEntity->mChildren )
    {
      EntityIterator treeEntityChildIterator = std::find(
        mEntities.begin(),
        mEntities.end(),
        treeEntityChild );
      treeIterators.push_back( treeEntityChildIterator );
    }
    iTreeIterator++;
  }

  for( EntityIterator treeIterator : treeIterators )
  {
    Entity* treeEntity = *treeIterator;
    if( Player* player = FindPlayer( treeEntity->mEntityUUID ) )
      player->mEntityUUID = NullEntityUUID;

    mEntities.erase( treeIterator );
    delete treeEntity;
  }
}

void               Tac::World::KillEntity( Entity* entity )
{
  auto it = std::find( mEntities.begin(), mEntities.end(), entity );
  KillEntity( it );
}

void               Tac::World::KillEntity( EntityUUID entityUUID )
{
  auto it = std::find_if( mEntities.begin(),
                          mEntities.end(),
                          [ & ]( Entity* entity ) { return entity->mEntityUUID == entityUUID; } );
  KillEntity( it );
}

Tac::Player*       Tac::World::SpawnPlayer( PlayerUUID playerUUID )
{
  auto player = TAC_NEW Player();
  player->mPlayerUUID = playerUUID;
  player->mWorld = this;
  mPlayers.PushFront( player );
  return player;
}

Tac::Player*       Tac::World::FindPlayer( PlayerUUID playerUUID )
{
  for( Player* player : mPlayers )
    if( player->mPlayerUUID == playerUUID )
      return player;
  return nullptr;
}

Tac::Player*       Tac::World::FindPlayer( EntityUUID entityUUID )
{
  for( Player* player : mPlayers )
    if( player->mEntityUUID == entityUUID )
      return player;
  return nullptr;
}

void               Tac::World::KillPlayer( PlayerUUID playerUUID )
{
  auto it = std::find_if( mPlayers.begin(),
                          mPlayers.end(),
                          [ & ]( Player* player ) { return player->mPlayerUUID == playerUUID; } );
  TAC_ASSERT( it != mPlayers.end() );
  auto player = *it;
  KillEntity( player->mEntityUUID );
  delete player;
  mPlayers.erase( it );
}

void               Tac::World::ApplyInput( Player* player, float seconds )
{
  TAC_UNUSED_PARAMETER( seconds );
  auto entity = FindEntity( player->mEntityUUID );
  if( !entity )
    return;
  // update velocity
  Collider*  collider = Collider::GetCollider( entity );
  if( !collider )
    return;
  float speedHorizontal = 5;
  float speedVertical = 7;
  // temp, align camera with player movement shit
  float hack = -1;
  float velX = player->mInputDirection.x * speedHorizontal;
  float velY = player->mIsSpaceJustDown ? speedVertical : collider->mVelocity.y;
  float velZ = player->mInputDirection.y * speedHorizontal * hack;
  collider->mVelocity = v3( velX, velY, velZ );

  // update rotation
  //auto stuff = ( Stuff* )entity->GetComponent( ComponentRegistryEntryIndex::Stuff );
  //stuff->mWaddleParams.Update( player->mInputDirection, seconds );
  //stuff->zCCWEulerRotDeg = stuff->mWaddleParams.mAngle;
}

void               Tac::World::ComputeTransformsRecursively( const m4& parentTransform,
                                                             Entity* entity )
{
  m4 localTransform = m4::Transform( entity->mRelativeSpace.mScale,
                                     entity->mRelativeSpace.mEulerRads,
                                     entity->mRelativeSpace.mPosition );
  m4 worldTransform = parentTransform * localTransform;

  m4 localTransformNoScale = m4::Transform( v3( 1, 1, 1 ),
                                            entity->mRelativeSpace.mEulerRads,
                                            entity->mRelativeSpace.mPosition );
  m4 worldTransformNoScale = parentTransform * localTransformNoScale;

  //entity->mLocalTransform = localTransform;
  entity->mWorldTransform = worldTransform;
  entity->mWorldPosition = ( worldTransform * v4( 0, 0, 0, 1 ) ).xyz();
  //entity->mWorldTransformNoScale = worldTransformNoScale;

  for( Entity* child : entity->mChildren )
  {
    const m4* parentTransformForChild = &worldTransform;
    if( !child->mInheritParentScale )
      parentTransformForChild = &worldTransformNoScale;
    ComputeTransformsRecursively( *parentTransformForChild, child );
  }
}

void               Tac::World::Step( float seconds )
{
  TAC_PROFILE_BLOCK;
  const m4 identity = m4::Identity();
  for( Entity* entity : mEntities )
  {
    if( !entity->mParent )
      ComputeTransformsRecursively( identity, entity );
  }

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
  if( mDebugDrawEntityOrigins )
  {
    auto boxSize = v3( 1, 1, 1 ) * 0.1f;
    v3 boxRot = {};
    Graphics* graphics = GetGraphics( this );
    if( graphics )
    {
      for( Entity* entity : mEntities )
      {
        TAC_UNUSED_PARAMETER( entity );
        //graphics->DebugDrawOBB( entity->mPosition, boxSize, boxRot );
      }
    }
  }
}

void               Tac::World::DeepCopy( const World& world )
{
  for( Player* player : mPlayers )
    delete player;

  mPlayers.Clear();

  for( Entity* entity : mEntities )
    delete entity;

  mEntities.Clear();

  mElapsedSecs = world.mElapsedSecs;

  for( Player* fromPlayer : world.mPlayers )
  {
    auto player = TAC_NEW Player;
    *player = *fromPlayer;
    mPlayers.PushFront( player );
  }

  for( Entity* fromEntity : world.mEntities )
  {
    Entity* toEntity = SpawnEntity( fromEntity->mEntityUUID );
    toEntity->DeepCopy( *fromEntity );
  }
}

Tac::System*       Tac::World::GetSystem( const SystemRegistryEntry* systemRegistryEntry )
{
  return mSystems[ systemRegistryEntry->mIndex ];
}

const Tac::System* Tac::World::GetSystem( const SystemRegistryEntry* systemRegistryEntry ) const
{
  return mSystems[ systemRegistryEntry->mIndex ];
}

void               Tac::World::DebugImgui()
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

