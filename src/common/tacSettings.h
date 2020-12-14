// For saving shit to a file

#pragma once

#include "src/common/tacJson.h"

namespace Tac
{
	struct     Errors;
	void       SettingsInit( Errors& );
	void       SettingsSave( Errors& );
	Json*      SettingsGetJson( StringView path, Json* root = nullptr );
	StringView SettingsGetString( StringView path, StringView fallback, Json* = nullptr );
	void       SettingsSetString( StringView path, StringView setValue, Json* = nullptr );
  JsonNumber SettingsGetNumber( StringView path, JsonNumber fallback, Json* = nullptr );
  void       SettingsSetNumber( StringView path, JsonNumber setValue, Json* = nullptr );
  bool       SettingsGetBool( StringView path, bool fallback, Json* = nullptr );
  void       SettingsSetBool( StringView path, bool setValue, Json* = nullptr );
}

