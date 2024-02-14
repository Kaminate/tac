#pragma once

#include "space/ecs/tac_component.h"
#include "src/common/string/tac_string.h"

namespace Tac
{
  struct Skybox : public Component
  {
    static Skybox*                 GetSkybox( Entity* );
    static const Skybox*           GetSkybox( const Entity* );
    const ComponentRegistryEntry*        GetEntry() const override;

    String                         mSkyboxDir;
  };


  void RegisterSkyboxComponent();

}
