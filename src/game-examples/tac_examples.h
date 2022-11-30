#pragma once
#include "src/common/graphics/tac_renderer.h"

namespace Tac
{
  struct Errors;
  struct World;
  struct Camera;
  struct v3;
  struct v2;

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
