#pragma once
#include "src/common/string/tac_string.h"

namespace Tac
{
  struct Errors;
  struct Example
  {
    virtual void Init( Errors& ){};
    virtual void Update( Errors& ){};
    virtual void Uninit( Errors& ){};
    virtual const char* GetName() const = 0;
    struct World* mWorld = nullptr;
    struct Camera* mCamera = nullptr;

  };


}
