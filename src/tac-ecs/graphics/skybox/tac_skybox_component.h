#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-std-lib/string/tac_string.h"

namespace Tac
{
  struct Skybox : public Component
  {
  static void RegisterComponent();

    static Skybox*                 GetSkybox( Entity* );
    static const Skybox*           GetSkybox( const Entity* );
    const ComponentRegistryEntry*  GetEntry() const override;

    String                         mSkyboxDir;
  };



}

