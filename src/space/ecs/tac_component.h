#pragma once

#include "src/common/dataprocess/tac_serialization.h" // NetworkBits
#include "src/common/tac_core.h"
#include "space/tac_space.h"

namespace Tac
{

  struct Component
  {
    virtual ~Component() = default;
    virtual void                          PreReadDifferences() {};
    virtual void                          PostReadDifferences() {};
    virtual const ComponentRegistryEntry* GetEntry() const = 0;
    Entity*                               mEntity = nullptr;
  };


}

