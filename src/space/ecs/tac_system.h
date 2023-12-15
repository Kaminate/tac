#pragma once

#include "space/tac_space.h"

namespace Tac
{
  struct System
  {
    virtual ~System() = default;
    virtual void DebugImgui() {};
    virtual void Update() {};
    World*       mWorld = nullptr;
  };


  struct SystemRegistryEntry
  {
    typedef System*     SystemCreateFn();
    typedef void        SystemDebugImGuiFn( System* );
    SystemCreateFn*     mCreateFn = nullptr;
    const char*         mName = nullptr;
    SystemDebugImGuiFn* mDebugImGui = nullptr;

    //                  Index of this system in the registry, also the 
    //                  index of this system in the world systems array
    int                 mIndex = -1;
  };

  struct SystemRegistryIterator
  {
    const SystemRegistryEntry* begin() const;
    const SystemRegistryEntry* end() const;
  };

  SystemRegistryEntry* SystemRegisterNewEntry();



}
