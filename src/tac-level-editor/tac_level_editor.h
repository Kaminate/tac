#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-level-editor/tac_entity_selection.h"
#include "tac-level-editor/tac_level_editor_window_manager.h"
#include "tac-level-editor/tac_level_editor_gizmo_mgr.h"
#include "tac-level-editor/tac_level_editor_sim_state.h"
#include "tac-level-editor/tac_level_editor_sys_state.h"

namespace Tac
{
  struct CreationAppState;

  struct Creation
  {
    void                 Init( SettingsNode, Errors& );
    void                 Uninit( Errors& );
    void                 Update( World*, Camera*, Errors& );
    void                 Render( const CreationAppState*, Errors& );

    // Entities
    RelativeSpace        GetEditorCameraVisibleRelativeSpace( const Camera* );
    Entity*              CreateEntity(World*,Camera*);
    Entity*              InstantiateAsCopy( World*,Camera*, Entity*, const RelativeSpace& );

    // ...
    SelectedEntities     mSelectedEntities        {};

    // Gizmos
    GizmoMgr             mGizmoMgr                {};


    EntityUUIDCounter    mEntityUUIDCounter       {};
    SettingsNode         mSettingsNode            {};
    CreationMousePicking mMousePicking            {};
    IconRenderer         sIconRenderer            {};
    WidgetRenderer       sWidgetRenderer          {};
    CreationSimState     mSimState                {};
    CreationSysState     mSysState                {};
  };

  //===-------------- Misc -----------------===//

  //                    If input path is absolute, it will be converted to be 
  //                    relative to the working directory
  //void                  ModifyPathRelative( FileSys::Path& path );


  extern Creation       gCreation;

} // namespace Tac

