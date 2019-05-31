#pragma once
#include "tacspacetypes.h"
#include "common/math/tacVector2.h"
#include "common/math/tacMatrix4.h"
#include "common/tacString.h"
#include <list>

struct TacGameInterface;
struct TacPlayer;
struct TacEntity;
struct TacSystem;
struct TacString;

static const int sPlayerCountMax = 4;

struct TacWorld
{
  TacWorld();
  ~TacWorld();

  TacSystem* GetSystem( TacSystemType systemType );
  void ClearPlayersAndEntities();
  void DeepCopy( const TacWorld& );
  void Step( float seconds );
  void DebugImgui();
  void ApplyInput( TacPlayer* player, float seconds );
  void ComputeTransformsRecursively( const m4& parentWorldTransformNoScale, TacEntity* entity );



  TacEntity* SpawnEntity( TacEntityUUID entityUUID );
  void KillEntity( TacEntityUUID entityUUID );
  TacEntity* FindEntity( TacPlayerUUID playerUUID );
  TacEntity* FindEntity( TacEntityUUID entityUUID );
  TacEntity* FindEntity( const TacString& name );

  TacPlayer* SpawnPlayer( TacPlayerUUID playerUUID );
  void KillPlayer( TacPlayerUUID playerUUID );
  TacPlayer* FindPlayer( TacPlayerUUID playerUUID );
  TacPlayer* FindPlayer( TacEntityUUID entityUUID );

  double mElapsedSecs = 0;
  bool mDebugDrawEntityOrigins = true;
  std::list< TacPlayer* > mPlayers;
  std::list< TacEntity* > mEntities;
  std::list< TacSystem* > mSystems;
  TacString mSkyboxDir;
};
