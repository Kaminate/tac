#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
  struct Shell
  {
    static String           sShellAppName;
    static String           sShellStudioName;
    static FileSys::Path    sShellPrefPath;
  };
} // namespace Tac
