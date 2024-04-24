#pragma once

#include "tac-ecs/tac_space.h"

namespace Tac
{
  struct System
  {
    virtual ~System() = default;
    virtual void DebugImgui() {};
    virtual void Update()     {};
    World*       mWorld       {};
  };

  struct SystemRegistryEntry
  {
    typedef System*     SystemCreateFn();
    typedef void        SystemDebugImGuiFn( System* );
    SystemCreateFn*     mCreateFn   {};
    const char*         mName       {};
    SystemDebugImGuiFn* mDebugImGui {};

    //                  Index of this system in the registry, also the 
    //                  index of this system in the world systems array
    int                 mIndex      { -1 };
  };

  struct SystemRegistryIterator
  {
    const SystemRegistryEntry* begin() const;
    const SystemRegistryEntry* end() const;
  };

  SystemRegistryEntry* SystemRegisterNewEntry();
}

