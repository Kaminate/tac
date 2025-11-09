#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-engine-core/settings/tac_settings_node.h"

namespace Tac
{
  struct Shell
  {
    static String           sShellAppName;
    static String           sShellStudioName;
    static FileSys::Path    sShellPrefPath;
    static SettingsNode     sShellSettings;
  };
} // namespace Tac
