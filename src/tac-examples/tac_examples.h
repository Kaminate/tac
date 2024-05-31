#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  struct SimKeyboardApi;
  struct SimWindowApi;

  struct Example
  {
    struct UpdateParams
    {
      const SimKeyboardApi* mKeyboardApi;
      const SimWindowApi* mWindowApi;
    };

    Example();
    virtual ~Example();
    virtual void Update( UpdateParams, Errors& ) {};
    v3           GetWorldspaceKeyboardDir();

    World*                mWorld       {};
    Camera*               mCamera      {};
    const char*           mName        {};
  };

}
