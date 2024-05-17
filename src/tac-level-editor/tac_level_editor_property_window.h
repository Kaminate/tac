#pragma once

#include "src/common/tac_core.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-ecs/tac_space.h"

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
    DesktopWindowHandle            mDesktopWindowHandle;
    bool                           mCloseRequested { false };
  };

  const char* const gPropertyWindowName = "PropertyWindow";

}
