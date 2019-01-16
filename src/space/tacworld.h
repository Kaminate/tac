#pragma once
#include "tacspacetypes.h"
#include "common/math/tacVector2.h"
#include <list>

struct TacGameInterface;
struct TacPlayer;
struct TacEntity;
struct TacSystem;

static const int sPlayerCountMax = 4;

struct TacWorld
{
  TacWorld();
  ~TacWorld();

  TacSystem* GetSystem( TacSystemType systemType );
  void ClearPlayersAndEntities();
  void DeepCopy( const TacWorld& );
  void EnsureAllEntityMatrixValidity();
  void Step( float seconds );
  void DebugImgui();
  void ApplyInput( TacPlayer* player, float seconds );

  TacEntity* SpawnEntity( TacEntityUUID entityUUID );
  void KillEntity( TacEntityUUID entityUUID );
  TacEntity* FindEntity( TacPlayerUUID playerUUID );
  TacEntity* FindEntity( TacEntityUUID entityUUID );

  TacPlayer* SpawnPlayer( TacPlayerUUID playerUUID );
  void KillPlayer( TacPlayerUUID playerUUID );
  TacPlayer* FindPlayer( TacPlayerUUID playerUUID );
  TacPlayer* FindPlayer( TacEntityUUID entityUUID );

  double mElapsedSecs = 0;
  bool mDebugDrawEntityOrigins = true;
  std::list< TacPlayer* > mPlayers;
  std::list< TacEntity* > mEntities;
  std::list< TacSystem* > mSystems;
};
