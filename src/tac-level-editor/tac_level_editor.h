#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/tac_space.h"
#include "tac-level-editor/tac_entity_selection.h"

namespace Tac
{
  // "Creation" is the name of the level editor
  struct Creation
  {
    static void Init( Errors& );
    static void Uninit( Errors& );
    static void Update( Errors& );
    static void Render( World*, const Camera*, Errors& );
    static auto GetEditorCameraVisibleRelativeSpace( const Camera* ) -> RelativeSpace;
    static auto CreateEntity() -> Entity*;
    static auto InstantiateAsCopy( Entity* ) -> Entity*;
    static auto GetEditorCamera() -> Camera*;
    static auto GetCamera() -> Camera*;
    static auto GetWorld() -> World*;
    static auto GetEntityUUIDCounter() -> EntityUUIDCounter*;
    static bool IsGameRunning();
  };


} // namespace Tac

