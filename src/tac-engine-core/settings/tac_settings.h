// For saving shit to a file

#pragma once

#include "tac-std-lib/dataprocess/tac_json.h"
//#include "tac-std-lib/tac_core.h"

namespace Tac
{

	void       SettingsInit( Errors& );

	//         Instructs that the settings file should be updated next write tick
	void       SettingsSave();
	Json*      SettingsGetJson( StringView path, Json* root = nullptr );
	Json*      SettingsGetChildByKeyValuePair( StringView key, const Json& value, Json* root);
	StringView SettingsGetString( StringView path, StringView fallback, Json* = nullptr );
	void       SettingsSetString( StringView path, StringView setValue, Json* = nullptr );
  JsonNumber SettingsGetNumber( StringView path, JsonNumber fallback, Json* = nullptr );
  void       SettingsSetNumber( StringView path, JsonNumber setValue, Json* = nullptr );
  bool       SettingsGetBool( StringView path, bool fallback, Json* = nullptr );
  void       SettingsSetBool( StringView path, bool setValue, Json* = nullptr );
  void       SettingsTick( Errors& );

	//         Write to save file now
  void       SettingsFlush( Errors& );
}

