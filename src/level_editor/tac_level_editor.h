#pragma once

#include "src/common/tac_core.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/containers/tac_vector.h"
#include "space/tac_space_types.h"
#include "space/tac_space.h"
#include "src/level_editor/tac_entity_selection.h"
#include "src/level_editor/tac_level_editor_window_manager.h"

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
    SelectedEntities    mSelectedEntities;
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
    EntityUUIDCounter   mEntityUUIDCounter;
    LevelEditorWindowManager mWindowManager;
  };

  //===-------------- Misc -----------------===//

  //                    If input path is absolute, it will be converted to be 
  //                    relative to the working directory
  //void                  ModifyPathRelative( Filesystem::Path& path );


  extern Creation       gCreation;

} // namespace Tac

