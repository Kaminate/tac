#pragma once

#include "src/common/tac_common.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/containers/tac_vector.h"
#include "src/space/tac_space_types.h"
#include "src/space/tac_space.h"
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
    World*              mWorld = nullptr;

    // Gizmos
    bool                mSelectedHitOffsetExists = false;
    v3                  mSelectedHitOffset = {};
    bool                mSelectedGizmo = false;
    v3                  mTranslationGizmoDir = {};
    float               mTranslationGizmoOffset = 0;

    // ...
    Camera*             mEditorCamera{};

    bool                mUpdateAssetView = false;
    EntityUUIDCounter   mEntityUUIDCounter;
    LevelEditorWindowManager mWindowManager;
  };

  //===-------------- Misc -----------------===//

  //                    If input path is absolute, it will be converted to be 
  //                    relative to the working directory
  //void                  ModifyPathRelative( Filesystem::Path& path );


  extern Creation       gCreation;

} // namespace Tac

