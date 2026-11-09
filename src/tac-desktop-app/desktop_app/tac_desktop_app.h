#pragma once

#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct DesktopApp
  {
    static void Run( Errors& );
    static void Update( Errors& );
    static void DebugImGui( Errors& );
    static auto GetMainErrors() -> Errors&;
  };

} // namespace Tac

