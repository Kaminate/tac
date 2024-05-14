#pragma once

#include "tac-std-lib/containers/tac_list.h"
#include "tac-desktop-app/desktop_app/tac_iapp.h"


namespace Tac { struct Errors; }

namespace Tac
{
  struct DesktopApp
  {
    void                Init( Errors& );
    void                Run( Errors& );
    void                Update( Errors& );
    void                DebugImGui( Errors& );
    static DesktopApp*  GetInstance();
    static Errors&      GetMainErrors();
  };

} // namespace Tac
