#pragma once


#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_math.h"

namespace Tac
{
  struct GizmoMgr
  {
    void Update( Ray, Errors& );
    bool IsTranslationWidgetActive( int ) const;
    void ComputeArrowLen();

    bool                mSelectedGizmo           {};
    v3                  mGizmoOrigin             {};
    bool                mTranslationGizmoVisible {};
    v3                  mTranslationGizmoDir     {};
    float               mTranslationGizmoOffset  {};
    int                 mTranslationGizmoAxis    {};
    bool                mGizmosEnabled           { true };
    float               mArrowLen                {};

    static GizmoMgr     sInstance;
  };

} // namespace Tac

