#pragma once

#include "src/common/graphics/tac_renderer.h"
#include "src/common/tac_common.h"
#include "src/space/tac_space.h"

namespace Tac
{
  struct Example
  {
    Example();
    virtual ~Example();
    virtual void Update( Errors& ){};
    v3 GetWorldspaceKeyboardDir();
    World* mWorld{};
    Camera* mCamera{};
    const char* mName{};
  };

}
