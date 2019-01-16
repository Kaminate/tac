#include "tacworld.h"
#include "tacplayer.h"
#include "tacentity.h"
#include "tacsystem.h"
#include "tacgraphics.h"
#include "tacphysics.h"
#include "taccollider.h"

#include "common/imgui.h"

#include <algorithm>

TacWorld::TacWorld()
{
  for( int i = 0; i < ( int )TacSystemType::Count; ++i )
  {
    auto systemType = ( TacSystemType )i;
    TacSystem* system = nullptr;
    switch( systemType )
    {
    case TacSystemType::Graphics: system = new TacGraphics(); break;
    case TacSystemType::Physics: system = new TacPhysics(); break;
    }
    if( !system )
      return;
    system->mWorld = this;
    mSystems.push_back( system );
  }
}
TacWorld::~TacWorld()
{
  ClearPlayersAndEntities();
  for( auto system : mSystems )
    delete system;
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
void TacWorld::KillEntity( TacEntityUUID entityUUID )
{
  auto it = std::find_if(
    mEntities.begin(),
    mEntities.end(),
    [ & ]( TacEntity* entity ) { return entity->mEntityUUID == entityUUID; } );
  TacAssert( it != mEntities.end() );
  TacEntity* entity = *it;
  delete entity;
  mEntities.erase( it );
  if( TacPlayer* player = FindPlayer( entityUUID ) )
    player->mEntityUUID = TacNullEntityUUID;
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
  auto collider = ( TacCollider* )entity->GetComponent( TacComponentType::Collider );
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
  //auto stuff = ( TacStuff* )entity->GetComponent( TacComponentType::Stuff );
  //stuff->mWaddleParams.Update( player->mInputDirection, seconds );
  //stuff->zCCWEulerRotDeg = stuff->mWaddleParams.mAngle;
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
  mElapsedSecs += seconds;
  if( mDebugDrawEntityOrigins )
  {
    auto boxSize = v3( 1, 1, 1 ) * 0.1f;
    v3 boxRot = {};
    auto graphics = ( TacGraphics* )GetSystem( TacSystemType::Graphics );
    if( graphics )
    {
      for( auto entity : mEntities )
      {
        graphics->DebugDrawOBB( entity->mPosition, boxSize, boxRot );
      }
    }
  }
}
void TacWorld::ClearPlayersAndEntities()
{
  for( auto player : mPlayers )
    delete player;
  mPlayers.clear();

  for( auto entity : mEntities )
    delete entity;
  mEntities.clear();
}
void TacWorld::DeepCopy( const TacWorld& world )
{
  ClearPlayersAndEntities();
  mElapsedSecs = world.mElapsedSecs;

  for( auto fromPlayer : world.mPlayers )
  {
    auto player = new TacPlayer();
    *player = *fromPlayer;
    mPlayers.push_back( player );
  }

  for( auto fromEntity : world.mEntities )
  {
    auto toEntity = SpawnEntity( fromEntity->mEntityUUID );
    toEntity->DeepCopy( *fromEntity );
  }
}
void TacWorld::EnsureAllEntityMatrixValidity()
{
  for( auto entity : mEntities )
  {
    //EnsureMatrixValidity();
  }
}
TacSystem* TacWorld::GetSystem( TacSystemType systemType )
{
  for( auto system : mSystems )
    if( system->GetSystemType() == systemType )
      return system;
  return nullptr;
}
void TacWorld::DebugImgui()
{
  if( !ImGui::CollapsingHeader( "World" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  if( ImGui::CollapsingHeader( "Entities" ) )
  {
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    for( auto entity : mEntities )
      entity->TacDebugImgui();
  }

  if( ImGui::CollapsingHeader( "Players" ) )
  {
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    for( auto player : mPlayers )
      player->DebugImgui();
  }

  for( auto system : mSystems )
    system->DebugImgui();

  ImGui::Checkbox( "Draw Entity Origins", &mDebugDrawEntityOrigins );

}
