#pragma once

#include "src/common/tac_common.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/space/tac_space.h"

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
    bool                           mCloseRequested = false;
  };

  const char* const gPropertyWindowName = "PropertyWindow";

}
