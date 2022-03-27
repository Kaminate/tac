#pragma once

#include "src/space/tacSpacetypes.h"
#include "src/common/math/tacVector2.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/string/tacString.h"
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

    System*       GetSystem( const SystemRegistryEntry* );
    const System* GetSystem( const SystemRegistryEntry* ) const;
    void          DeepCopy( const World& );
    void          Step( float seconds );
    void          DebugImgui();
    void          ApplyInput( Player*, float seconds );
    void          ComputeTransformsRecursively( const m4& parentWorldTransformNoScale, Entity* );

    //            Entity api
    Entity*       SpawnEntity( EntityUUID );
    void          KillEntity( EntityUUID );
    void          KillEntity( Entity* );
    void          KillEntity( EntityIterator );
    Entity*       FindEntity( PlayerUUID );
    Entity*       FindEntity( EntityUUID );
    Entity*       FindEntity( StringView );

    //            Player api
    Player*       SpawnPlayer( PlayerUUID );
    void          KillPlayer( PlayerUUID );
    Player*       FindPlayer( PlayerUUID );
    Player*       FindPlayer( EntityUUID );

    double               mElapsedSecs = 0;
    bool                 mDebugDrawEntityOrigins = true;
    std::list< Player* > mPlayers;
    std::list< Entity* > mEntities;
    Vector< System* >    mSystems;

    //                   The world owns a debug draw data so that
    //                   each system can access it
    Debug3DDrawData*     mDebug3DDrawData = nullptr;
  };
}
