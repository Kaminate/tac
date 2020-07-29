#include "src/common/tacSettings.h"
#include "src/common/tacUtility.h"
#include "src/common/tacMemory.h"
#include "src/common/tacOS.h"
#include "src/common/tacTemporaryMemory.h"

namespace Tac
{
  void Settings::Load( Errors& errors )
  {
    if( !FileExist( mPath ) )
    {
      Save( errors );
      TAC_HANDLE_ERROR( errors );
      return;
    }
    auto temporaryMemory = TemporaryMemoryFromFile( mPath, errors );
    mJson.Parse( temporaryMemory.data(), ( int )temporaryMemory.size(), errors );
  }
  void Settings::Save( Errors& errors )
  {
    String str = mJson.Stringify();
    OS::SaveToFile( mPath, ( void* )str.data(), ( int )str.size(), errors );
    TAC_HANDLE_ERROR( errors );
  }

  void Settings::GetSetting( Json* settingTree,
                             const SettingPath& settingVariables,
                             int iPath,
                             Json** outputSetting,
                             const Json& defaultValue,
                             Errors& errors )
  {
    if( !settingTree )
      settingTree = &mJson;
    const StringView settingVariable = settingVariables[ iPath ];
    Json& child = settingTree->operator[]( settingVariable );
    bool isLeaf = iPath == ( int )settingVariables.size() - 1;
    if( isLeaf )
    {
      if( child.mType == JsonType::Null )
      {
        child = defaultValue;
        Save( errors );
        if( errors )
        {
          String getSettingsErrors;
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

  void Settings::SetSetting( Json* settingTree,
                             const SettingPath& paths,
                             int iPath,
                             const Json& value,
                             Errors& errors )
  {
    if( !settingTree )
      settingTree = &mJson;
    const StringView settingVariable = paths[ iPath ];
    Json& child = settingTree->operator[]( settingVariable );
    bool isLeaf = iPath == ( int )paths.size() - 1;
    if( isLeaf )
    {
      child = value;
      Save( errors );
      if( errors )
      {
        String getSettingsErrors;
        getSettingsErrors += "Failed to save default setting ";
        getSettingsErrors += settingVariable;
        getSettingsErrors += value.Stringify();
        errors.mMessage += getSettingsErrors;
        return;
      }
      return;
    }
    SetSetting( &child, paths, iPath + 1, value, errors );
  }

  bool Settings::GetBool( Json* root,
                          const SettingPath& paths,
                          bool defaultValue,
                          Errors& errors )
  {
    Json* setting;
    GetSetting( root, paths, 0, &setting, Json( defaultValue ), errors );
    return setting->mBoolean;
  }

  JsonNumber Settings::GetNumber( Json* root,
                                  const SettingPath& paths,
                                  JsonNumber defaultValue,
                                  Errors& errors )
  {
    Json* setting;
    GetSetting( root, paths, 0, &setting, Json( defaultValue ), errors );
    return setting->mNumber;
  }

  void Settings::SetNumber( Json* root,
                            const SettingPath& paths,
                            JsonNumber value,
                            Errors& errors )
  {
    SetSetting( root, paths, 0, Json( value ), errors );
  }

  StringView Settings::GetString( Json* root,
                                  const SettingPath& paths,
                                  String defaultValue,
                                  Errors& errors )
  {
    Json* setting;
    GetSetting( root, paths, 0, &setting, Json( defaultValue ), errors );
    return setting->mString;
  }

  void Settings::SetString( Json* root,
                            const SettingPath& paths,
                            StringView value,
                            Errors& errors )
  {
    SetSetting( root, paths, 0, value, errors );
  }

  Json* Settings::GetArray( Json* root,
                            const SettingPath& paths,
                            Json* defaultValue,
                            Errors& errors )
  {
    Json emptyArray;
    emptyArray.mType = JsonType::Array;
    if( !defaultValue )
      defaultValue = &emptyArray;

    Json* setting;
    GetSetting( root, paths, 0, &setting, *defaultValue, errors );
    return setting;
  }

  Json* Settings::GetObject( Json* root,
                             const SettingPath& paths,
                             Json* defaultValue,
                             Errors& errors )
  {
    Json emptyObject;
    emptyObject.mType = JsonType::Object;
    if( !defaultValue )
      defaultValue = &emptyObject;

    Json* setting;
    GetSetting( root, paths, 0, &setting, *defaultValue, errors );
    return setting;
  }

}

