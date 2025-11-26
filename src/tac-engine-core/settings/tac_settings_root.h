// For saving shit to a file

#pragma once

#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
  struct SettingsRoot
  {
    void Init( const UTF8Path&, Errors& );
    auto GetRootNode() -> SettingsNode;
    void Tick( Errors& );
    void Flush( Errors& );
    void SetDirty();
    Json     sJson;
    bool     sDirty{};
    GameTime sLastSaveSeconds;
    UTF8Path sSavePath;
  };
}

