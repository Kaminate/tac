#pragma once

#include "tac-std-lib/containers/tac_list.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"


namespace Tac { struct Errors; }

namespace Tac
{
  struct DesktopApp
  {
    static void         Init( Errors& );
    static void         Run( Errors& );
    static void         Update( Errors& );
    static void         DebugImGui( Errors& );
    static Errors&      GetMainErrors();
  };

} // namespace Tac
