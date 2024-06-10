#pragma once


#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{

  struct GizmoMgr
  {
    void                Init( Errors& );
    void                Uninit();
    void                Update( Errors& );
    void                Render( Errors& );

    bool                mSelectedHitOffsetExists {};
    v3                  mSelectedHitOffset       {};
    bool                mSelectedGizmo           {};
    v3                  mTranslationGizmoDir     {};
    float               mTranslationGizmoOffset  {};

  };

} // namespace Tac

