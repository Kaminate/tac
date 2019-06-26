#include "space/tacworld.h"
#include "space/tacplayer.h"
#include "space/tacentity.h"
#include "space/tacsystem.h"
#include "space/tacgraphics.h"
#include "space/tacphysics.h"
#include "space/taccollider.h"

#include "common/graphics/tacDebug3D.h"

#include <algorithm>

TacWorld::TacWorld()
{
  TacSystemRegistry* registry = TacSystemRegistry::Instance();
  for( TacSystemRegistryEntry* entry : registry->mEntries )
  {
    TacSystem* system = entry->mCreateFn();
    TacAssert( system );
    system->mWorld = this;
    mSystems.push_back( system );
  }
  mDebug3DDrawData = new TacDebug3DDrawData;
}
TacWorld::~TacWorld()
{
  for( auto system : mSystems )
    delete system;
  for( auto player : mPlayers )
    delete player;
  for( auto entity : mEntities )
    delete entity;
}
TacEntity* TacWorld::SpawnEntity( TacEntityUUID entityUUID )
{
  auto entity = new TacEntity();
  entity->mEntityUUID = entityUUID;
  entity->mWorld = this;
  mEntities.push_back( entity );
  return entity;
}
TacEntity* TacWorld::FindEntity( TacPlayerUUID playerUUID )
{
  if( TacPlayer* player = FindPlayer( playerUUID ) )
    return FindEntity( player->mEntityUUID );
  return nullptr;
}
TacEntity* TacWorld::FindEntity( TacEntityUUID entityUUID )
{
  for( auto entity : mEntities )
    if( entity->mEntityUUID == entityUUID )
      return entity;
  return nullptr;
}
TacEntity* TacWorld::FindEntity( const TacString& name )
{
  for( TacEntity* entity : mEntities )
    if( entity->mName == name )
      return entity;
  return nullptr;
}
void TacWorld::KillEntity( TacEntityIterator it )
{
  TacAssert( it != mEntities.end() );
  TacEntity* entity = *it;

  // Remove this entity from its parent's list of children
  if( TacEntity* parent = entity->mParent )
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
    TacAssert( iEntity < entityCount );
    parent->mChildren[ iEntity ] = parent->mChildren[ entityCount - 1 ];
    parent->mChildren.pop_back();
  }

  TacVector< TacEntityIterator > treeIterators = { it };
  int iTreeIterator = 0;
  for( ;; )
  {
    if( iTreeIterator == treeIterators.size() )
      break;
    TacEntityIterator treeIterator = treeIterators[ iTreeIterator ];
    TacEntity* treeEntity = *treeIterator;
    for( TacEntity* treeEntityChild : treeEntity->mChildren )
    {
      TacEntityIterator treeEntityChildIterator = std::find(
        mEntities.begin(),
        mEntities.end(),
        treeEntityChild );
      treeIterators.push_back( treeEntityChildIterator );
    }
    iTreeIterator++;
  }

  for( TacEntityIterator treeIterator : treeIterators )
  {
    TacEntity* treeEntity = *treeIterator;
    if( TacPlayer* player = FindPlayer( treeEntity->mEntityUUID ) )
      player->mEntityUUID = TacNullEntityUUID;

    mEntities.erase( treeIterator );
    delete treeEntity;
  }
}
void TacWorld::KillEntity( TacEntity* entity )
{
  auto it = std::find(
    mEntities.begin(),
    mEntities.end(),
    entity );
  KillEntity( it );
}
void TacWorld::KillEntity( TacEntityUUID entityUUID )
{
  auto it = std::find_if(
    mEntities.begin(),
    mEntities.end(),
    [ & ]( TacEntity* entity ) { return entity->mEntityUUID == entityUUID; } );
  KillEntity( it );
}
TacPlayer* TacWorld::SpawnPlayer( TacPlayerUUID playerUUID )
{
  auto player = new TacPlayer();
  player->mPlayerUUID = playerUUID;
  player->mWorld = this;
  mPlayers.push_back( player );
  return player;
}
TacPlayer* TacWorld::FindPlayer( TacPlayerUUID playerUUID )
{
  for( auto player : mPlayers )
    if( player->mPlayerUUID == playerUUID )
      return player;
  return nullptr;
}
TacPlayer* TacWorld::FindPlayer( TacEntityUUID entityUUID )
{
  for( auto player : mPlayers )
    if( player->mEntityUUID == entityUUID )
      return player;
  return nullptr;
}
void TacWorld::KillPlayer( TacPlayerUUID playerUUID )
{
  auto it = std::find_if(
    mPlayers.begin(),
    mPlayers.end(),
    [ & ]( TacPlayer* player ) { return player->mPlayerUUID == playerUUID; } );
  TacAssert( it != mPlayers.end() );
  auto player = *it;
  KillEntity( player->mEntityUUID );
  delete player;
  mPlayers.erase( it );
}
void TacWorld::ApplyInput( TacPlayer* player, float seconds )
{
  auto entity = FindEntity( player->mEntityUUID );
  if( !entity )
    return;
  // update velocity
  TacCollider*  collider = TacCollider::GetCollider( entity );
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
  //auto stuff = ( TacStuff* )entity->GetComponent( TacComponentRegistryEntryIndex::Stuff );
  //stuff->mWaddleParams.Update( player->mInputDirection, seconds );
  //stuff->zCCWEulerRotDeg = stuff->mWaddleParams.mAngle;
}
void TacWorld::ComputeTransformsRecursively( const m4& parentWorldTransformNoScale, TacEntity* entity )
{
  m4 localTransform = M4Transform( entity->mLocalScale, entity->mLocalEulerRads, entity->mLocalPosition );
  m4 localTransformNoScale = M4Transform( v3( 1, 1, 1 ), entity->mLocalEulerRads, entity->mLocalPosition );
  m4 worldTransform = parentWorldTransformNoScale * localTransform;
  m4 worldTransformNoScale = parentWorldTransformNoScale * localTransformNoScale;

  entity->mLocalTransform = localTransform;
  entity->mWorldTransform = worldTransform;
  entity->mWorldTransformNoScale = worldTransformNoScale;

  for( TacEntity* child : entity->mChildren )
  {
    ComputeTransformsRecursively( worldTransformNoScale, child );
  }
}
void TacWorld::Step( float seconds )
{
  for( auto player : mPlayers )
    ApplyInput( player, seconds );
  for( auto system : mSystems )
    system->Update();
  for( auto entity : mEntities )
  {
    //entity->TacIntegrate( seconds );
  }

  const m4 identity = m4::Identity();
  for( TacEntity* entity : mEntities )
  {
    if( !entity->mParent )
      ComputeTransformsRecursively( identity, entity );
  }

  mElapsedSecs += seconds;
  if( mDebugDrawEntityOrigins )
  {
    auto boxSize = v3( 1, 1, 1 ) * 0.1f;
    v3 boxRot = {};
    TacGraphics* graphics = TacGraphics::GetSystem( this );
    if( graphics )
    {
      for( auto entity : mEntities )
      {
        //graphics->DebugDrawOBB( entity->mPosition, boxSize, boxRot );
      }
    }
  }
}
void TacWorld::DeepCopy( const TacWorld& world )
{
  for( auto player : mPlayers )
    delete player;
  mPlayers.clear();

  for( auto entity : mEntities )
    delete entity;
  mEntities.clear();

  mElapsedSecs = world.mElapsedSecs;
  mSkyboxDir = world.mSkyboxDir;

  for( TacPlayer* fromPlayer : world.mPlayers )
  {
    TacPlayer*  player = new TacPlayer();
    *player = *fromPlayer;
    mPlayers.push_back( player );
  }

  for( TacEntity* fromEntity : world.mEntities )
  {
    TacEntity* toEntity = SpawnEntity( fromEntity->mEntityUUID );
    toEntity->DeepCopy( *fromEntity );
  }
}
TacSystem* TacWorld::GetSystem( TacSystemRegistryEntry* systemRegistryEntry )
{
  return mSystems[ systemRegistryEntry->mIndex ];
}
void TacWorld::DebugImgui()
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
  //    entity->TacDebugImgui();
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
