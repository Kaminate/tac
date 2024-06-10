#include "tac_level_editor_gizmo_mgr.h" // self-inc

#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-ecs/entity/tac_entity.h"


namespace Tac
{

  void                GizmoMgr::Init( Errors& errors )
  {
  }
  void                GizmoMgr::Uninit()
  {
  }
  void                GizmoMgr::Update( Errors& errors )
  {
    if( !gCreation.mSelectedGizmo )
      return;

    SimKeyboardApi keyboardApi{};

    const v3 origin { gCreation.mSelectedEntities.GetGizmoOrigin() };
    float gizmoMouseDist;
    float secondDist;
    ClosestPointTwoRays( gCreation.mEditorCamera->mPos,
                         mWorldSpaceMouseDir,
                         origin,
                         gCreation.mTranslationGizmoDir,
                         &gizmoMouseDist,
                         &secondDist );

    const v3 translate {
      gCreation.mTranslationGizmoDir
      * ( secondDist - gCreation.mTranslationGizmoOffset ) };

    for( Entity* entity : gCreation.mSelectedEntities )
    {
      entity->mRelativeSpace.mPosition += translate;
    }

    if( !keyboardApi.IsPressed( Key::MouseLeft ) )
    {
      gCreation.mSelectedGizmo = false;
    }
  }

  void                GizmoMgr::Render( Errors& errors )
  {
  }


} // namespace Tac

