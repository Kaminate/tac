// For saving shit to a file

#pragma once

#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_timestamp.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

namespace Tac
{
  struct SettingsRoot
  {
    void Init( const FileSys::Path&, Errors& );
    auto GetRootNode() -> SettingsNode;
    void Tick( Errors& );
    void Flush( Errors& );
    void SetDirty();

    Json             sJson;
    bool             sDirty{};
    Timestamp        sLastSaveSeconds;
    FileSys::Path    sSavePath;
  };
}

