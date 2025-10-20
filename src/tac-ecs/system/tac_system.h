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
      auto begin() const -> const SystemInfo*;
      auto end() const -> const SystemInfo*;
    };

    using SystemCreateFn = System* ( * )();
    using SystemImGuiFn = void ( * )( System* );

    static auto Register() -> dynmc SystemInfo*;
    static auto Find( StringView ) -> const SystemInfo*;

    auto GetIndex() const -> int; // Indexes the [system registry] and the [world systems array]

    const char*         mName       {};
    SystemCreateFn      mCreateFn   {};
    SystemImGuiFn       mDebugImGui {};
  };


} // namespace Tac

