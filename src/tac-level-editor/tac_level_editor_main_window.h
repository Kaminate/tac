#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{
  struct CreationMainWindow
  {
    static void Update( World* , Errors& );

    static bool sShowWindow;
  };
}

