#pragma once

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_list.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac { struct m4; struct Debug3DDrawData; }

namespace Tac
{

  static constexpr int sPlayerCountMax{ 4 };

  using EntityIterator = List< Entity* >::Iterator;

  struct World
  {
    World();
    ~World();

    dynmc System* GetSystem( const SystemInfo* ) dynmc;
    const System* GetSystem( const SystemInfo* ) const;
    void          DeepCopy( const World& );
    void          Step( float seconds );
    void          DebugImgui();
    void          ApplyInput( Player*, float seconds );
    void          ComputeTransformsRecursively();
    void          ComputeTransformsRecursively( const m4& parentWorldTransformNoScale, Entity* );

    //            Entity api
    Entity*       SpawnEntity( EntityUUID );
    void          KillEntity( EntityUUID );
    void          KillEntity( Entity* );
    void          KillEntity( EntityIterator );
    Entity*       FindEntity( PlayerUUID );
    Entity*       FindEntity( EntityUUID );
    Entity*       FindEntity( StringView ); // returns first matching Entity::mName

    //            Player api
    Player*       SpawnPlayer( PlayerUUID );
    void          KillPlayer( PlayerUUID );
    Player*       FindPlayer( PlayerUUID );
    Player*       FindPlayer( EntityUUID );

    Timestamp            mElapsedSecs;
    bool                 mDebugDrawEntityOrigins { true };
    List< Player* >      mPlayers;
    List< Entity* >      mEntities;
    Vector< System* >    mSystems;

    //                   The world owns a debug draw data so that
    //                   each system can access it
    Debug3DDrawData*     mDebug3DDrawData {};
  };

  struct WorldsToDiff{ World* mOldWorld; World* mNewWorld; };

} // namespace Tac

