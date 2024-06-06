#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"
#include "tac-level-editor/tac_entity_selection.h"
#include "tac-level-editor/tac_level_editor_window_manager.h"

namespace Tac
{

  struct Creation
  {
    void                Init( Errors& );
    void                Uninit( Errors& );
    void                Update( Errors& );

    // Entities
    RelativeSpace       GetEditorCameraVisibleRelativeSpace();
    Entity*             CreateEntity();
    Entity*             InstantiateAsCopy( Entity*, const RelativeSpace& );

    // ...
    SelectedEntities    mSelectedEntities        {};
    World*              mWorld                   {};

    // Gizmos
    bool                mSelectedHitOffsetExists {};
    v3                  mSelectedHitOffset       {};
    bool                mSelectedGizmo           {};
    v3                  mTranslationGizmoDir     {};
    float               mTranslationGizmoOffset  {};

    // ...
    Camera*             mEditorCamera            {};

    bool                mUpdateAssetView         {};
    EntityUUIDCounter   mEntityUUIDCounter       {};
    LevelEditorWindowManager mWindowManager      {};
  };

  //===-------------- Misc -----------------===//

  //                    If input path is absolute, it will be converted to be 
  //                    relative to the working directory
  //void                  ModifyPathRelative( Filesystem::Path& path );


  extern Creation       gCreation;

} // namespace Tac

