#pragma once

#include "src/common/string/tac_string.h"
#include "space/tac_space.h"
#include "src/common/system/tac_desktop_window.h"

namespace Tac { struct Errors; }
namespace Tac
{

  struct CreationProfileWindow
  {
    CreationProfileWindow();
    ~CreationProfileWindow();
    static CreationProfileWindow* Instance;
    void                          Init( Errors& );
    void                          Update( Errors& );
    void                          ImGui();
    DesktopWindowHandle           mDesktopWindowHandle;
    bool                          mCloseRequested { false };
  };

  const char* const gProfileWindowName = "ProfileWindow";

}

