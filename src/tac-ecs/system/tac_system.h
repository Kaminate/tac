#pragma once

#include "tac-ecs/tac_space.h"
#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

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

    static dynmc SystemInfo*  Register();
    static const SystemInfo*  Find( StringView );

    //                  Index of this system in the registry, also the 
    //                  index of this system in the world systems array
    int                 GetIndex() const;

    const char*         mName       {};
    SystemCreateFn      mCreateFn   {};
    SystemImGuiFn       mDebugImGui {};

  };


} // namespace Tac

