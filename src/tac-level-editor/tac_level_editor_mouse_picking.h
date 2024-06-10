#pragma once


#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/light/tac_light.h"

namespace Tac
{

  struct CreationMousePicking
  {
    void                          BeginFrame();
    void                          Update();

  private:
    void                          MousePickingEntityLight( const Light*,
                                                           bool* hit,
                                                           float* dist );
    void                          MousePickingEntityModel( const Model*,
                                                           bool* hit,
                                                           float* dist );
    void                          MousePickingEntity( const Entity*,
                                                      bool* hit,
                                                      float* dist );
    void                          MousePickingEntities();
    void                          MousePickingGizmos();
    void                          MousePickingSelection();

    v3                            mViewSpaceUnitMouseDir    {};
    v3                            mWorldSpaceMouseDir       {};
  };

} // namespace Tac

