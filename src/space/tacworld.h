#pragma once
#include "tacspacetypes.h"
#include "common/math/tacVector2.h"
#include "common/math/tacMatrix4.h"
#include "common/tacString.h"
#include "common/containers/tacVector.h"
#include "common/containers/tacArray.h"
#include <list>

struct TacGameInterface;
struct TacPlayer;
struct TacEntity;
struct TacSystem;
struct TacString;
struct TacWorld;
struct TacShell;
struct TacSystemRegistryEntry;
struct TacDebug3DDrawData;

static const int sPlayerCountMax = 4;

typedef std::list< TacEntity* >::iterator TacEntityIterator;


struct TacWorld
{
  TacWorld();
  ~TacWorld();

  TacSystem* GetSystem( TacSystemRegistryEntry* );
  void DeepCopy( const TacWorld& );
  void Step( float seconds );
  void DebugImgui();
  void ApplyInput( TacPlayer* player, float seconds );
  void ComputeTransformsRecursively( const m4& parentWorldTransformNoScale, TacEntity* entity );

  // Entity api
  TacEntity* SpawnEntity( TacEntityUUID entityUUID );
  void KillEntity( TacEntityUUID entityUUID );
  void KillEntity( TacEntity* entity );
  void KillEntity( TacEntityIterator it );
  TacEntity* FindEntity( TacPlayerUUID playerUUID );
  TacEntity* FindEntity( TacEntityUUID entityUUID );
  TacEntity* FindEntity( const TacString& name );

  // Player api
  TacPlayer* SpawnPlayer( TacPlayerUUID playerUUID );
  void KillPlayer( TacPlayerUUID playerUUID );
  TacPlayer* FindPlayer( TacPlayerUUID playerUUID );
  TacPlayer* FindPlayer( TacEntityUUID entityUUID );

  double mElapsedSecs = 0;
  bool mDebugDrawEntityOrigins = true;
  std::list< TacPlayer* > mPlayers;
  std::list< TacEntity* > mEntities;
  TacVector< TacSystem* > mSystems;
  TacString mSkyboxDir;
  TacShell* mShell = nullptr;
  TacDebug3DDrawData* mDebug3DDrawData = nullptr;
};

