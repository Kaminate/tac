#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{
  struct Skybox : public Component
  {
    static void RegisterComponent();
    static auto GetSkybox( dynmc Entity* ) -> dynmc Skybox*;
    static auto GetSkybox( const Entity* ) -> const Skybox*;
    auto GetEntry() const -> const ComponentInfo* override;
    String mSkyboxDir;
  };
}

