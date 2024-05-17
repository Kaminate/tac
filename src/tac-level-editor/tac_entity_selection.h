#pragma once

#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-ecs/tac_space_types.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{
  struct SelectedEntities
  {
    bool                empty() const;
    int                 size() const;
    Entity**            begin();
    Entity**            end();
    v3                  GetGizmoOrigin() const;
    void                clear();
    void                DeleteEntitiesCheck();
    void                DeleteEntities();
    void                AddToSelection( Entity* );
    void                Select( Entity* );
    bool                IsSelected( Entity* );

    Vector< Entity* >   mSelectedEntities;
  };


} // namespace Tac

