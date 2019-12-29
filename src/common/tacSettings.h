// Super convenient way of saving shit to a file

#pragma once

#include "tacJson.h"
#include "tacErrorHandling.h"

typedef TacVector< TacString > TacSettingPath;

struct TacSettings
{
  TacJson mJson;
  TacString mPath;
  void Load( TacErrors& errors );
  void Save( TacErrors& errors );

  // These all have the property where it calls TacSettings::Save
  bool GetBool(
    TacJson* root,
    const TacSettingPath& paths,
    bool defaultValue,
    TacErrors& errors );

  TacJsonNumber GetNumber(
    TacJson* root,
    const TacSettingPath& paths,
    TacJsonNumber defaultValue,
    TacErrors& errors );

  void SetNumber(
    TacJson* root,
    const TacSettingPath& paths,
    TacJsonNumber value,
    TacErrors& errors );

  TacString GetString(
    TacJson* root,
    const TacSettingPath& paths,
    TacString defaultValue,
    TacErrors& errors );

  TacJson* GetArray(
    TacJson* root,
    const TacSettingPath& paths,
    TacJson* defaultValue,
    TacErrors& errors );

  TacJson* GetObject(
    TacJson* root,
    const TacSettingPath& paths,
    TacJson* defaultValue,
    TacErrors& errors ); 

private:

  void GetSetting(
    TacJson* settingTree,
    const TacSettingPath& paths,
    int iPath,
    TacJson** outputSetting,
    const TacJson& defaultValue,
    TacErrors& errors );
  void SetSetting(
    TacJson* settingTree,
    const TacSettingPath& paths,
    int iPath,
    const TacJson& value,
    TacErrors& errors );
};

