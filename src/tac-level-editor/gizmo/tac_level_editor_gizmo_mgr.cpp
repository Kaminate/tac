#include "tac_level_editor_gizmo_mgr.h" // self-inc
#include "tac_level_editor.h" // self-inc

#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-level-editor/tools/tac_level_editor_tool.h"
#include "tac-level-editor/tools/tac_level_editor_selection_tool.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-std-lib/math/tac_vector4.h"

namespace Tac
{
  GizmoMgr GizmoMgr::sInstance;

  bool GizmoMgr::IsTranslationWidgetActive( int i ) const
  {
    return mGizmosEnabled && mSelectedGizmo && mTranslationGizmoAxis == i;
  }

  void GizmoMgr::ComputeArrowLen()
  {
    if( SelectedEntities::empty() )
    {
      mArrowLen = 0;
      return;
    }

    const Camera* camera{ Creation::GetCamera() };
    const m4 view{ m4::View( camera->mPos, camera->mForwards, camera->mRight, camera->mUp ) };
    const v3 pos{ SelectedEntities::ComputeAveragePosition() };
    const v4 posVS4{ view * v4( pos, 1 ) };
    const float clip_height{ Abs( Tan( camera->mFovyrad / 2.0f ) * posVS4.z * 2.0f ) };
    const float arrowLen{ clip_height * 0.2f };
    mArrowLen = arrowLen;
  }

  void GizmoMgr::Update( Ray worldspaceMouseRay, Errors& )
  {
    if( !SelectedEntities::empty() )
    {
      mTranslationGizmoVisible = Toolbox::GetActiveTool() == &SelectionTool::sInstance;

      mGizmoOrigin = SelectedEntities::ComputeAveragePosition();
    }

    if( !mSelectedGizmo )
      return;

    if( !AppKeyboardApi::IsPressed( Key::MouseLeft ) )
    {
      mSelectedGizmo = false;
      return;
    }

    const ClosestPointTwoRays output(
      {
        .mRay0 { worldspaceMouseRay },
        .mRay1 {.mOrigin { mGizmoOrigin }, .mDirection{mTranslationGizmoDir} },
      } );
    const float secondDist{ output.mRay1T };
    const v3 translate { mTranslationGizmoDir * ( secondDist - mTranslationGizmoOffset ) };
    for( Entity* entity : SelectedEntities() )
      entity->mRelativeSpace.mPosition += translate;
  }

} // namespace Tac

