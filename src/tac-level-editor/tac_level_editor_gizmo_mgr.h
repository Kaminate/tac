#pragma once


#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-level-editor/tac_entity_selection.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  struct GizmoMgr
  {
    void                Update( v3, const Camera*, Errors& );
    void                Render( Errors& );
    bool                IsTranslationWidgetActive( int );
    void                ComputeArrowLen( const Camera* );

    bool                mSelectedGizmo           {};
    v3                  mGizmoOrigin             {};
    bool                mTranslationGizmoVisible {};
    v3                  mTranslationGizmoDir     {};
    float               mTranslationGizmoOffset  {};
    int                 mTranslationGizmoAxis    {};
    bool                mGizmosEnabled           { true };
    float               mArrowLen                {};
  };

} // namespace Tac

