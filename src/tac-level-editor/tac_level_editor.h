#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-level-editor/tac_entity_selection.h"

namespace Tac
{
  struct CreationAppState;

  struct Creation
  {
    void Init( SettingsNode, Errors& );
    void Uninit( Errors& );
    void Update( Errors& );
    void Render( World*, Camera*, Errors& );
    auto GetEditorCameraVisibleRelativeSpace( const Camera* ) -> RelativeSpace;
    auto CreateEntity(World*,Camera*) -> Entity*;
    auto InstantiateAsCopy( World*,Camera*, Entity*, const RelativeSpace& ) -> Entity*;

    EntityUUIDCounter    mEntityUUIDCounter       {};
    SettingsNode         mSettingsNode            {};
    World*               mWorld                   {};
    Camera*              mEditorCamera            {};
    static Creation      gCreation;
  };


} // namespace Tac

