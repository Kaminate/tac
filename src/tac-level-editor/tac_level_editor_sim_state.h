#pragma once

#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{

  struct CreationSimState
  {
    ~CreationSimState();

    void Init( Errors& );
    void Uninit();
    void Clear();
    void CopyFrom( CreationSimState& );

    void operator = ( const CreationSimState& ) = delete;


    World*  mWorld        {};
    Camera* mEditorCamera {};
  };

} // namespace Tac

