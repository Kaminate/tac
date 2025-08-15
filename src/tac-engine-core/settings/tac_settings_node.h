// For saving shit to a file

#pragma once

#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/containers/tac_span.h"

namespace Tac::FileSys { struct Path; }
namespace Tac
{
  struct SettingsRoot;

  struct SettingsNode
  {
    SettingsNode() = default;
    SettingsNode( SettingsRoot*, Json* );
    auto GetChild( StringView ) -> SettingsNode;
    auto GetChildrenArray() -> Span< SettingsNode >;
    void SetValue( Json );
    auto GetValueWithFallback( Json = {} ) -> Json&;
    auto GetValue() -> Json&;
    bool IsValid() const;

  private:
    SettingsRoot* mRoot{};
    Json*         mJson{};
  };
}

