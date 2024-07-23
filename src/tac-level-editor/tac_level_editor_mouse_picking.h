#pragma once

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-level-editor/tac_entity_selection.h"
#include "tac-level-editor/tac_level_editor_gizmo_mgr.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct CreationMousePicking
  {
    void                          BeginFrame( WindowHandle, Camera* );
    void                          Init( SelectedEntities*, GizmoMgr*, Errors& );
    void                          Update( const World*, const Camera* );
    bool                          IsTranslationWidgetPicked( int );
    v3                            GetWorldspaceMouseDir() const;

  private:

    void                          MousePickingEntities( const World*, const Camera* );
    void                          MousePickingGizmos( const Camera*);
    void                          MousePickingSelection( const Camera*);

    v3                            mViewSpaceUnitMouseDir    {};
    v3                            mWorldSpaceMouseDir       {};
    SelectedEntities*             mSelectedEntities         {};
    GizmoMgr*                     mGizmoMgr                 {};
    Mesh*                         mArrow                    {};
    bool                          mWindowHovered            {};
  };

} // namespace Tac

