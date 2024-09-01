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

  struct SystemInfo
  {
    struct Iterate
    {
      const SystemInfo* begin() const;
      const SystemInfo* end() const;
    };

    using SystemCreateFn = System* ( * )();
    using SystemImGuiFn = void ( * )( System* );

    static SystemInfo*  Register();

    const char*         mName       {};
    SystemCreateFn      mCreateFn   {};
    SystemImGuiFn       mDebugImGui {};

    //                  Index of this system in the registry, also the 
    //                  index of this system in the world systems array
    int                 mIndex      { -1 };
  };


} // namespace Tac

