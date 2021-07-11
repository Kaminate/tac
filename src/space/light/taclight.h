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
  struct Light : public Component
  {
    static Light*                        GetLight( Entity* );
    static const Light*                  GetLight( const Entity* );
    const ComponentRegistryEntry*        GetEntry() const override;
    enum Type
    {
      kDirectional,
      kSpot,
    };

    float                     mSpotHalfFOVRadians = 0.5f;
    Type                      mType = kSpot;
    bool                      mCastsShadows = true;
    int                       mShadowResolution = 512;
    Render::TextureHandle     mShadowMap;
    Render::FramebufferHandle mShadowFrame;
  };

  void                                   RegisterLightComponent();
}

