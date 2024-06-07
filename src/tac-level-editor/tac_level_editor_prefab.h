#pragma once

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/settings/tac_settings_node.h"

namespace Tac { struct AssetPathStringView; struct Errors; }
namespace Tac
{

  void                PrefabSave( World*, Errors& );
  void                PrefabLoadAtPath( SettingsNode,
                                        EntityUUIDCounter*,
                                        World*,
                                        Camera*,
                                        const AssetPathStringView&,
                                        Errors& );

  //                  TODO: when prefabs spawn, they need to have a entity uuid.
  //                        this can only be done by a serverdata.
  //
  //                        So this function should also have a serverdata parameter
  void                PrefabLoad( SettingsNode, EntityUUIDCounter*, World*, Camera*, Errors& );
  void                PrefabImGui();


  //void                PrefabLoadCamera( Prefab* );
  //void                PrefabLoadCameraVec( Prefab*, StringView, v3& );
  void                PrefabSaveCamera( SettingsNode, Camera* );
  void                PrefabRemoveEntityRecursively( SettingsNode,Entity* );
  AssetPathStringView PrefabGetOrNull( Entity* );

  AssetPathStringView PrefabGetLoaded();
} // namespace Tac

