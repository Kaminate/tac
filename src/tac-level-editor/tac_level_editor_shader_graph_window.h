#pragma once

#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct CreationShaderGraphWindow
  {
    static void                    Update( Errors& );
    static bool                    sShowWindow;
  };
}
