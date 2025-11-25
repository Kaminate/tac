#pragma once

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/shell/tac_shell_game_time.h"
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

    void Init();
    auto GetSystem( const SystemInfo* ) dynmc -> dynmc System*;
    auto GetSystem( const SystemInfo* ) const -> const System*;
    void DeepCopy( const World& );
    void Step( TimeDelta );
    void DebugImgui();
    void ApplyInput( Player*, TimeDelta );
    void ComputeTransformsRecursively();

    //   Entity api
    auto SpawnEntity( EntityUUID ) -> Entity*;
    void KillEntity( EntityUUID );
    void KillEntity( Entity* );
    void KillEntity( EntityIterator );
    auto FindEntity( PlayerUUID ) -> Entity*;
    auto FindEntity( EntityUUID ) -> Entity*;
    auto FindEntity( StringView ) -> Entity*;

    //   Player api
    auto SpawnPlayer( PlayerUUID ) -> Player*;
    void KillPlayer( PlayerUUID );
    auto FindPlayer( PlayerUUID ) -> Player*;
    auto FindPlayer( EntityUUID ) -> Player*;

    GameTime            mElapsedSecs;
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

