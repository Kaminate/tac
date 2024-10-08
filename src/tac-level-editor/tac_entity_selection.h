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
    void Init( SettingsNode );
    void                DeleteEntitiesCheck();
    void                DeleteEntities();
    void                AddToSelection( Entity* );
    void                Select( Entity* );
    bool                IsSelected( Entity* );
    v3                  ComputeAveragePosition();

    bool                empty() const;
    int                 size() const;
    Entity**            begin();
    Entity**            end();
    void                clear();

    Vector< Entity* >   mSelectedEntities;
    SettingsNode        mSettingsNode;
  };


} // namespace Tac

