// For saving shit to a file

#pragma once

#include "tac-std-lib/dataprocess/tac_json.h"

namespace Tac::FileSys { struct Path; }
namespace Tac
{
  struct SettingsRoot;

  struct SettingsNode
  {
    SettingsNode() = default;
    SettingsNode( SettingsRoot*, Json* );
    SettingsNode GetChild( StringView );
    void         SetValue( Json );
    Json&        GetValueWithFallback( Json = {} );
    Json&        GetValue();

  private:
    SettingsRoot* mRoot{};
    Json*         mJson{};
  };
}

