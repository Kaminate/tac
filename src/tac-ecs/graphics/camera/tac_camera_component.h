#pragma once

#include "tac-ecs/component/tac_component.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"

namespace Tac
{
  struct CameraComponent : public Component
  {
    static void RegisterComponent();
    static auto GetCamera( dynmc Entity* ) -> dynmc CameraComponent*;
    static auto GetCamera( const Entity* ) -> const CameraComponent*;
    auto GetEntry() const -> const ComponentInfo* override;
    Camera mCamera;
  };

  TAC_META_DECL( CameraComponent );
}

