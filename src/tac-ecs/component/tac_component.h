#pragma once

#include "tac-std-lib/dataprocess/tac_serialization.h" // NetworkBits
#include "tac-ecs/tac_space.h"

namespace Tac
{

  struct Component
  {
    virtual ~Component() = default;
    virtual void                          PreReadDifferences() {};
    virtual void                          PostReadDifferences() {};
    virtual const ComponentRegistryEntry* GetEntry() const = 0;
    Entity*                               mEntity {};
  };


}

