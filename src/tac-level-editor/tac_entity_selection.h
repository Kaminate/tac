#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{
  struct SelectedEntities
  {
    struct Iterator
    {
      void operator ++();
      bool operator != ( Iterator& ) const;
      auto operator* () const -> Entity*;
      Entity** mBegin {};
      Entity** mEnd   {};
      Entity** mCurr  {};
    };
    auto begin() -> Iterator;
    auto end() -> Iterator;
    static void Init( SettingsNode );
    static void DeleteEntitiesCheck();
    static void DeleteEntities();
    static void AddToSelection( Entity* );
    static void Select( Entity* );
    static bool IsSelected( Entity* );
    static auto ComputeAveragePosition() -> v3;
    static bool empty();
    static void clear();
  };


} // namespace Tac

