
// Super convenient way of saving shit to a file

#pragma once

#include "src/common/tacJson.h"
#include "src/common/tacErrorHandling.h"

namespace Tac
{
typedef Vector< String > SettingPath;

struct Settings
{
  Json mJson;
  String mPath;
  void Load( Errors& errors );
  void Save( Errors& errors );

  // These all have the property where it calls Settings::Save
  bool GetBool(
    Json* root,
    const SettingPath& paths,
    bool defaultValue,
    Errors& errors );

  JsonNumber GetNumber(
    Json* root,
    const SettingPath& paths,
    JsonNumber defaultValue,
    Errors& errors );

  void SetNumber(
    Json* root,
    const SettingPath& paths,
    JsonNumber value,
    Errors& errors );

  StringView GetString(
    Json* root,
    const SettingPath& paths,
    String defaultValue,
    Errors& errors );

  Json* GetArray(
    Json* root,
    const SettingPath& paths,
    Json* defaultValue,
    Errors& errors );

  Json* GetObject(
    Json* root,
    const SettingPath& paths,
    Json* defaultValue,
    Errors& errors ); 

private:

  void GetSetting(
    Json* settingTree,
    const SettingPath& paths,
    int iPath,
    Json** outputSetting,
    const Json& defaultValue,
    Errors& errors );
  void SetSetting(
    Json* settingTree,
    const SettingPath& paths,
    int iPath,
    const Json& value,
    Errors& errors );
};


}

