#pragma once

#include "src/space/tac_space_types.h"
#include "src/space/tac_space.h"
#include "src/common/tac_common.h"

namespace Tac
{

  void                PrefabSave( World* );
  void                PrefabLoadAtPath( EntityUUIDCounter*, World*, Camera*, String, Errors& );

  //                  TODO: when prefabs spawn, they need to have a entity uuid.
  //                        this can only be done by a serverdata.
  //
  //                        So this function should also have a serverdata parameter
  void                PrefabLoad( EntityUUIDCounter*, World*, Camera*, Errors& );
  void                PrefabImGui();


  //void                PrefabLoadCamera( Prefab* );
  //void                PrefabLoadCameraVec( Prefab*, StringView, v3& );
  void                PrefabSaveCamera( Camera* );
  void                PrefabRemoveEntityRecursively( Entity* );
  const char*         PrefabGetOrNull( Entity* );

  StringView          PrefabGetLoaded();
}

