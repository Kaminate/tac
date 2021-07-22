#pragma once

#include "src/common/graphics/tacRenderer.h"
#include "src/space/tacComponent.h"

// Okay but like
// arent we already making too many generalizations?
//
// whoose to say that all lights are going to be used the same?
// like we are writing the structUre for a light before the
// pipeline for key/secondary/indirect lights exist
//
namespace Tac
{
  struct Camera;
  struct Light : public Component
  {
    static Light*                        GetLight( Entity* );
    static const Light*                  GetLight( const Entity* );
    const ComponentRegistryEntry*        GetEntry() const override;
    void                                 FreeRenderResources();
    Camera                               GetCamera() const;
    v3                                   GetUnitDirection() const;

    enum Type
    {
      kDirectional,
      kSpot,
      kCount,
    };

    bool                      mOverrideClipPlanes = false;
    float                     mFarPlaneOverride = 10000.0f;
    float                     mNearPlaneOverride = 0.1f;
    float                     mSpotHalfFOVRadians = 0.5f;
    Type                      mType = kSpot;
    bool                      mCastsShadows = true;
    int                       mShadowResolution = 512;

    bool                      mCreatedRenderResources = false;
    //Render::TextureHandle     mShadowMapColor;
    Render::TextureHandle     mShadowMapDepth;
    Render::FramebufferHandle mShadowFramebuffer;
    Render::ViewHandle        mShadowView;
  };

  void                                   RegisterLightComponent();
  const char*                            LightTypeToString( Light::Type );
  Light::Type                            LightTypeFromString( const char* );
}

