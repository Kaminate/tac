#include "common/tacSettings.h"
#include "common/tacUtility.h"
#include "common/tacMemory.h"
#include "common/tacOS.h"

void TacSettings::Load( TacErrors& errors )
{
  if( !TacFileExist( mPath ) )
  {
    errors = "File doesn't exist at path: " + mPath;
    return;
  }
  auto temporaryMemory = TacTemporaryMemory( mPath, errors );
  mJson.Parse( temporaryMemory.data(), ( int )temporaryMemory.size(), errors );
}
void TacSettings::Save( TacErrors& errors )
{
  auto str = mJson.Stringify();
  //TacSaveToFile( mPath, ( void* )str.data(), ( int )str.size(), errors );
  TacOS::Instance->SaveToFile( mPath, ( void* )str.data(), ( int )str.size(), errors );
  TAC_HANDLE_ERROR( errors );
}

void TacSettings::GetSetting(
  TacJson* settingTree,
  const TacVector< TacString >& settingVariables,
  int iPath,
  TacJson** outputSetting,
  const TacJson& defaultValue,
  TacErrors& errors )
{
  if( !settingTree )
    settingTree = &mJson;
  const TacString& settingVariable = settingVariables[ iPath ];
  TacJson& child = settingTree->operator[]( settingVariable );
  bool isLeaf = iPath == ( int )settingVariables.size() - 1;
  if( isLeaf )
  {
    if( child.mType == TacJsonType::Null )
    {
      child = defaultValue;
      Save( errors );
      if( errors.size() )
      {
        TacString getSettingsErrors;
        getSettingsErrors += "Failed to save default setting ";
        getSettingsErrors += settingVariable;
        getSettingsErrors += defaultValue.Stringify();
        errors.mMessage += getSettingsErrors;
        return;
      }
    }
    *outputSetting = &child;
    return;
  }
  GetSetting( &child, settingVariables, iPath + 1, outputSetting, defaultValue, errors );
}

/*

TacString TacSettings::GetSettingz( const TacString& result, const TacVector< TacString >& paths, TacErrors& errors )
{
  return GetString( result, paths, errors );
}
TacString TacSettings::GetString( const TacString& result, const TacVector< TacString >& paths, TacErrors& errors )
{
  return GetSetting( paths, TacJson( result ), errors )->mString;
}
TacJsonNumber TacSettings::GetSettingz( TacJsonNumber result, const TacVector< TacString >& paths, TacErrors& errors )
{
  return GetNumber( result, paths, errors );
}
TacJsonNumber TacSettings::GetNumber( TacJsonNumber result, const TacVector< TacString >& paths, TacErrors& errors )
{
  return GetSetting( paths, TacJson( result ), errors )->mNumber;
}
bool TacSettings::GetSettingz( bool result, const TacVector< TacString >& paths, TacErrors& errors )
{
  return GetBool( result, paths, errors );
}
bool TacSettings::GetBool( bool result, const TacVector< TacString >& paths, TacErrors& errors )
{
}
TacJson* TacSettings::GetArray( const TacVector< TacString >& paths, TacErrors& errors )
{
  TacJson defaultValue;
  defaultValue.mType = TacJsonType::Array;
  return GetSetting( paths, defaultValue, errors );
}

TacJson* TacSettings::GetObject(
  const TacVector< TacString >& paths,
  const TacJson& defaultValue,
  TacErrors& errors )
{
  TacJson* result;
  GetSetting( mJson, paths, 0, &result, defaultValue, errors );
  return result;
}

*/

bool TacSettings::GetBool(
  TacJson* root,
  const TacVector< TacString >& paths,
  bool defaultValue,
  TacErrors& errors )
{
  TacJson* setting;
  GetSetting( root, paths, 0, &setting, TacJson( defaultValue ), errors );
  return setting->mBoolean;
}

TacJsonNumber TacSettings::GetNumber(
  TacJson* root,
  const TacVector< TacString >& paths,
  TacJsonNumber defaultValue,
  TacErrors& errors )
{
  TacJson* setting;
  GetSetting( root, paths, 0, &setting, TacJson( defaultValue ), errors );
  return setting->mNumber;
}

TacString TacSettings::GetString(
  TacJson* root,
  const TacVector< TacString >& paths,
  TacString defaultValue,
  TacErrors& errors )
{
  TacJson* setting;
  GetSetting( root, paths, 0, &setting, TacJson( defaultValue ), errors );
  return setting->mString;
}

TacJson* TacSettings::GetArray(
  TacJson* root,
  const TacVector< TacString >& paths,
  TacJson* defaultValue,
  TacErrors& errors )
{
  TacJson emptyArray;
  emptyArray.mType = TacJsonType::Array;
  if( !defaultValue )
    defaultValue = &emptyArray;
  
  TacJson* setting;
  GetSetting( root, paths, 0, &setting, *defaultValue, errors );
  return setting;
}

TacJson* TacSettings::GetObject(
  TacJson* root,
  const TacVector< TacString >& paths,
  TacJson* defaultValue,
  TacErrors& errors )
{
  TacJson emptyObject;
  emptyObject.mType = TacJsonType::Object;
  if( !defaultValue )
    defaultValue = &emptyObject;

  TacJson* setting;
  GetSetting( root, paths, 0, &setting, *defaultValue, errors );
  return setting;
}
