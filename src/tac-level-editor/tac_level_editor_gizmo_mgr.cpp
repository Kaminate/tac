#include "tac_level_editor_gizmo_mgr.h" // self-inc

#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-std-lib/math/tac_vector4.h"


namespace Tac
{

  void                GizmoMgr::Init( SelectedEntities* selectedEntities, Errors& errors )
  {
    mSelectedEntities = selectedEntities;
  }

  void                GizmoMgr::Uninit()
  {
  }

  bool                GizmoMgr::IsTranslationWidgetActive( int i )
  {
    return mGizmosEnabled && mSelectedGizmo && mTranslationGizmoAxis == i;
  }

  void GizmoMgr::ComputeArrowLen( const Camera* camera )
  {
    if( mSelectedEntities->empty() )
    {
      mArrowLen = 0;
      return;
    }

    const m4 view{ m4::View( camera->mPos,
                             camera->mForwards,
                             camera->mRight,
                             camera->mUp ) };
    const v3 pos{ mSelectedEntities->ComputeAveragePosition() };
    const v4 posVS4{ view * v4( pos, 1 ) };
    const float clip_height{ Abs( Tan( camera->mFovyrad / 2.0f )
                                   * posVS4.z
                                   * 2.0f ) };
    const float arrowLen{ clip_height * 0.2f };
    mArrowLen = arrowLen;
  }

  void                GizmoMgr::Update( const v3 worldSpaceMouseDir,
                                        const Camera* camera,
                                        Errors& errors )
  {
    if( !mSelectedEntities->empty() )
    {
      mTranslationGizmoVisible = true;
      mGizmoOrigin = mSelectedEntities->ComputeAveragePosition();
    }


    if( !mSelectedGizmo )
      return;

    if( !AppKeyboardApi::IsPressed( Key::MouseLeft ) )
    {
      mSelectedGizmo = false;
      return;
    }

    const ClosestPointTwoRays::Input input
    {
      .mRay0Pos { camera->mPos },
      .mRay0Dir { worldSpaceMouseDir },
      .mRay1Pos { mGizmoOrigin },
      .mRay1Dir { mTranslationGizmoDir },
    };
    const ClosestPointTwoRays::Output output{ ClosestPointTwoRays::Solve( input ) };
    const float gizmoMouseDist{ output.mRay0T };
    const float secondDist{ output.mRay1T };

    const v3 translate {
      mTranslationGizmoDir
      * ( secondDist - mTranslationGizmoOffset ) };

    for( Entity* entity : *mSelectedEntities )
    {
      entity->mRelativeSpace.mPosition += translate;
    }
  }

  void                GizmoMgr::Render( Errors& errors )
  {
  }


} // namespace Tac

