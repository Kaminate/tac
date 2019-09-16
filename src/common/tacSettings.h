// Super convenient way of saving shit to a file

#pragma once

#include "tacJson.h"
#include "tacErrorHandling.h"

struct TacSettings
{
  TacJson mJson;
  TacString mPath;
  void Load( TacErrors& errors );
  void Save( TacErrors& errors );

  // TODO:
  //   these functions are useful for things outside of settings ( prefab loading ).
  //   Move them into tacjson.h
  bool GetBool(
    TacJson* root,
    const TacVector< TacString >& paths,
    bool defaultValue,
    TacErrors& errors );

  TacJsonNumber GetNumber(
    TacJson* root,
    const TacVector< TacString >& paths,
    TacJsonNumber defaultValue,
    TacErrors& errors );

  void SetNumber(
    TacJson* root,
    const TacVector< TacString >& paths,
    TacJsonNumber value,
    TacErrors& errors );

  TacString GetString(
    TacJson* root,
    const TacVector< TacString >& paths,
    TacString defaultValue,
    TacErrors& errors );

  TacJson* GetArray(
    TacJson* root,
    const TacVector< TacString >& paths,
    TacJson* defaultValue,
    TacErrors& errors );

  TacJson* GetObject(
    TacJson* root,
    const TacVector< TacString >& paths,
    TacJson* defaultValue,
    TacErrors& errors ); 

private:

  void GetSetting(
    TacJson* settingTree,
    const TacVector< TacString >& paths,
    int iPath,
    TacJson** outputSetting,
    const TacJson& defaultValue,
    TacErrors& errors );
  void SetSetting(
    TacJson* settingTree,
    const TacVector< TacString >& paths,
    int iPath,
    const TacJson& value,
    TacErrors& errors );
};

