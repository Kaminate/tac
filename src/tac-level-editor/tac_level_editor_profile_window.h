#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-ecs/tac_space.h"
#include "tac-engine-core/window/tac_window_handle.h"

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
    WindowHandle                  mWindowHandle;
    bool                          mCloseRequested { false };
  };

  const char* const gProfileWindowName { "ProfileWindow" };

}

