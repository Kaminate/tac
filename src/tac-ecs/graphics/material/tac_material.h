#pragma once

#include "tac-rhi/render3/tac_render_api.h" // Render::TextureHandle
#include "tac-ecs/component/tac_component.h"

namespace Tac { struct Camera; struct StringView; }
namespace Tac::Render { struct ShaderMaterial; }

namespace Tac
{
  struct Material : public Component
  {
    static Material*              GetMaterial( Entity* );
    static const Material*        GetMaterial( const Entity* );
    const ComponentRegistryEntry* GetEntry() const override;

    static void                   RegisterComponent();
    static void                   DebugImgui( Material* );

    // ...
  };

}

