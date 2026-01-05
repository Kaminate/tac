#pragma once

#include "tac-rhi/render3/tac_render_api.h" // Render::TextureHandle
//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-std-lib/meta/tac_meta_decl.h"

// Okay but like
// arent we already making too many generalizations?
//
// whoose to say that all lights are going to be used the same?
// like we are writing the structUre for a light before the
// pipeline for key/secondary/indirect lights exist
//

namespace Tac { struct Camera; struct StringView; }
namespace Tac::Render { struct ShaderLight; }

namespace Tac
{
  struct Light : public Component
  {
    enum Type
    {
      kDirectional,

      // A spotlight gives out a lot of power in one direction, and less power in other directions
      kSpot,
      kCount,
    };

    static void RegisterComponent();
    static auto GetLight( dynmc Entity* ) -> dynmc Light*;
    static auto GetLight( const Entity* ) -> const Light*;

    auto GetEntry() const -> const ComponentInfo* override;
    void FreeRenderResources();
    auto GetCamera() const->Camera;
    auto GetUnitDirection() const ->v3;
    bool                      mOverrideClipPlanes     {};
    float                     mFarPlaneOverride       { 10000.0f };
    float                     mNearPlaneOverride      { 0.1f };
    float                     mSpotHalfFOVRadians     { 0.5f };
    Type                      mType                   { kSpot };
    bool                      mCastsShadows           { true };
    int                       mShadowResolution       { 512 };
    bool                      mCreatedRenderResources {};

    // this is a texture with a depth stencil view binding. ( depth buffer ) 
    // basically this will be the only thing bound to the framebuffer, without color buffers.
    Render::TextureHandle     mShadowMapDepth;
    v3                        mColor                  { 1, 1, 1 } ;
    float                     mRadiance               { 5.0f }; // i guess?
  };

  auto LightToShaderLight( const Light* ) -> Render::ShaderLight;
  auto LightTypeToString( Light::Type ) -> const char*;
  auto LightTypeFromString( StringView ) -> Light::Type;
  auto LightDebugImgui( Light* ) -> void;

  TAC_META_DECL( Light );
}

