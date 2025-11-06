#pragma once

#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/asset/tac_asset.h" // AssetPathStringView

namespace Tac
{
  bool PrefabSave( World*, Errors& );
  void PrefabLoadAtPath(
    SettingsNode, EntityUUIDCounter*, World*, Camera*, const AssetPathStringView&, Errors& );
  void PrefabLoad( SettingsNode, EntityUUIDCounter*, World*, Camera*, Errors& );
  void PrefabImGui();
  void PrefabSaveCamera( SettingsNode, Camera* );
  void PrefabRemoveEntityRecursively( SettingsNode,Entity* );
  auto PrefabGetOrNull( Entity* ) -> AssetPathStringView;
  auto PrefabGetLoaded() -> AssetPathStringView;
} // namespace Tac

