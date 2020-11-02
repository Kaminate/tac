
// Super convenient way of saving shit to a file

#pragma once

#include "src/common/tacJson.h"

namespace Tac
{
  typedef Vector< String > SettingPath;
  struct Errors;

  struct Settings
  {
    Settings();
    ~Settings();
    static Settings* Instance;
    void             Init( Errors& );
    Json             mJson;
    String           mPath;
    void             Load( Errors& );
    void             Save( Errors& );

    // These all have the property where it calls Settings::Save
    bool             GetBool( Json* root,
                              const SettingPath& paths,
                              bool defaultValue,
                              Errors& );

    JsonNumber       GetNumber( Json* root,
                                      const SettingPath& paths,
                                      JsonNumber defaultValue,
                                      Errors& );

    void SetNumber( Json* root,
                    const SettingPath& paths,
                    JsonNumber value,
                    Errors& );

    StringView GetString( Json* root,
                          const SettingPath& paths,
                          String defaultValue,
                          Errors& );

    void SetString( Json* root,
                    const SettingPath& paths,
                    StringView value,
                    Errors& );

    Json* GetArray( Json* root,
                    const SettingPath& paths,
                    Json* defaultValue,
                    Errors& );

    Json* GetObject( Json* root,
                     const SettingPath& paths,
                     Json* defaultValue,
                     Errors& );

  private:

    void GetSetting( Json* settingTree,
                     const SettingPath& paths,
                     int iPath,
                     Json** outputSetting,
                     const Json& defaultValue,
                     Errors& );
    void SetSetting( Json* settingTree,
                     const SettingPath& paths,
                     int iPath,
                     const Json& value,
                     Errors& );
  };


}

