#pragma once

#include "src/space/tacComponent.h"
#include "src/common/tacString.h"

namespace Tac
{
  struct Skybox : public Component
  {
    static Skybox*                 GetSkybox( Entity* );
    static const Skybox*           GetSkybox( const Entity* );
    ComponentRegistryEntry*        GetEntry() const override;

    String                         mSkyboxDir;
  };


  void RegisterSkyboxComponent();

}

