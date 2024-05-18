#pragma once

#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/tac_space.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{

  struct CreationPropertyWindow
  {
    CreationPropertyWindow();
    ~CreationPropertyWindow();
    static CreationPropertyWindow* Instance;
    void                           Init( Errors& );
    void                           Update( Errors& );
    void                           RecursiveEntityHierarchyElement( Entity* );
    WindowHandle            mWindowHandle;
    bool                           mCloseRequested { false };
  };

  const char* const gPropertyWindowName = "PropertyWindow";

}
