#pragma once

#include "tac-std-lib/dataprocess/tac_serialization.h" // NetVars
#include "tac-ecs/tac_space.h"

namespace Tac
{

  struct Component
  {
    virtual ~Component() = default;
    virtual void PreReadDifferences() {};
    virtual void PostReadDifferences() {};
    virtual auto GetEntry() const -> const ComponentInfo* = 0;
    void         CopyFrom( const Component* );

    Entity* mEntity {};
  };


} // namespace Tac

