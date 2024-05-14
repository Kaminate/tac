// For saving shit to a file

#pragma once

#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_timestamp.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"


namespace Tac::FileSys { struct Path; }
namespace Tac
{
  struct SettingsRoot
  {
    //void          GetSetting( StringView ); //  separated by dots

    void         Init( const FileSys::Path&, Errors& );
    SettingsNode GetRootNode();
    void         Tick( Errors& );
    void         Flush( Errors& );
    void         SetDirty();

    /*
    Json*      GetJson( StringView path, Json* root = nullptr );
    Json*      GetChildByKeyValuePair( StringView key, const Json& value, Json* root );
    void       SetString( StringView path, StringView setValue, Json* = nullptr );
    JsonNumber GetNumber( StringView path, JsonNumber fallback, Json* = nullptr );
    void       SetNumber( StringView path, JsonNumber setValue, Json* = nullptr );
    bool       GetBool( StringView path, bool fallback, Json* = nullptr );
    void       SetBool( StringView path, bool setValue, Json* = nullptr );
    */

    Json             sJson;
    bool             sDirty{};
    Timestamp        sLastSaveSeconds;
    FileSys::Path    sSavePath;
  };
}

