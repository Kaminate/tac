#pragma once

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac
{
  struct CreationMousePicking
  {
    static void BeginFrame( WindowHandle );
    static void Init( Errors& );
    static void Update( Errors& );
    static bool IsTranslationWidgetPicked( int );
    static auto GetWorldspaceMouseRay() -> Ray;
    static auto GetPickedEntity() -> Entity*;
    static auto GetPickedPos() -> v3;
    static bool sDrawRaycast;
  };

} // namespace Tac

