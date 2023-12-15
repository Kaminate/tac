#pragma once

#include "src/common/tac_core.h"
#include "src/common/math/tac_vector3.h"
#include "src/common/containers/tac_vector.h"
#include "space/tac_space_types.h"
#include "space/tac_space.h"

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

