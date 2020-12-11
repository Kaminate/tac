// For saving shit to a file

#pragma once

#include "src/common/tacJson.h"

namespace Tac
{
	struct Errors;

	void                    SettingsInit( Errors& );
	void                    SettingsSave( Errors& );
	Json*                   SettingsGetJson( StringView path, Json* root = nullptr );
	StringView              SettingsGetString( StringView path, StringView fallback, Json* = nullptr );
	StringView              SettingsGetString( Json*, StringView fallback );
	StringView              SettingsSetString( Json*, StringView fallback );
	JsonNumber              SettingsGetNumber( Json*, JsonNumber fallback );
	JsonNumber              SettingsSetNumber( Json*, JsonNumber fallback );
	bool                    SettingsGetBool( Json*, bool );
	bool                    SettingsSetBool( Json*, bool );
}

