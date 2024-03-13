#pragma once

#include "space/tac_space_types.h"
#include "space/tac_space.h"
#include "tac-std-lib/shell/tac_shell_timestep.h"
#include "tac-std-lib/tac_core.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_list.h"

namespace Tac
{
  struct Debug3DDrawData;

  static const int sPlayerCountMax = 4;

  typedef List< Entity* >::Iterator EntityIterator;

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
    Entity* SpawnEntity( EntityUUID );
    void    KillEntity( EntityUUID );
    void    KillEntity( Entity* );
    void    KillEntity( EntityIterator );
    Entity* FindEntity( PlayerUUID );
    Entity* FindEntity( EntityUUID );
    Entity* FindEntity( StringView );

    //            Player api
    Player* SpawnPlayer( PlayerUUID );
    void    KillPlayer( PlayerUUID );
    Player* FindPlayer( PlayerUUID );
    Player* FindPlayer( EntityUUID );

    Timestamp            mElapsedSecs;
    bool                 mDebugDrawEntityOrigins = true;
    List< Player* >      mPlayers;
    List< Entity* >      mEntities;
    Vector< System* >    mSystems;

    //                   The world owns a debug draw data so that
    //                   each system can access it
    Debug3DDrawData* mDebug3DDrawData = nullptr;
  };
}

