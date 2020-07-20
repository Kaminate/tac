
#pragma once
#include "src/space/tacSpacetypes.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include "src/common/containers/tacArray.h"
#include <list>


namespace Tac
{
struct GameInterface;
struct Player;
struct Entity;
struct System;
struct String;
struct World;
struct Shell;
struct SystemRegistryEntry;
struct Debug3DDrawData;

static const int sPlayerCountMax = 4;

typedef std::list< Entity* >::iterator EntityIterator;


struct World
{
  World();
  ~World();

  System* GetSystem( SystemRegistryEntry* );
  void DeepCopy( const World& );
  void Step( float seconds );
  void DebugImgui();
  void ApplyInput( Player* player, float seconds );
  void ComputeTransformsRecursively( const m4& parentWorldTransformNoScale, Entity* entity );

  // Entity api
  Entity* SpawnEntity( EntityUUID entityUUID );
  void KillEntity( EntityUUID entityUUID );
  void KillEntity( Entity* entity );
  void KillEntity( EntityIterator it );
  Entity* FindEntity( PlayerUUID playerUUID );
  Entity* FindEntity( EntityUUID entityUUID );
  Entity* FindEntity( StringView name );

  // Player api
  Player* SpawnPlayer( PlayerUUID playerUUID );
  void KillPlayer( PlayerUUID playerUUID );
  Player* FindPlayer( PlayerUUID playerUUID );
  Player* FindPlayer( EntityUUID entityUUID );

  double mElapsedSecs = 0;
  bool mDebugDrawEntityOrigins = true;
  std::list< Player* > mPlayers;
  std::list< Entity* > mEntities;
  Vector< System* > mSystems;
  String mSkyboxDir;
  Debug3DDrawData* mDebug3DDrawData = nullptr;
};


}

