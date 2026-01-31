#pragma once

#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/tac_space.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"

namespace Tac
{

  template< typename T > struct ComponentVisitor { virtual void operator()( T* ) = 0; };
  using ModelVisitor    = ComponentVisitor< Model >;
  using SkyboxVisitor   = ComponentVisitor< Skybox >;
  using LightVisitor    = ComponentVisitor< Light >;
  using MaterialVisitor = ComponentVisitor< Material >;
  using CameraVisitor   = ComponentVisitor< CameraComponent >;
  using NumGridVisitor  = ComponentVisitor< NumGrid >;
  using Sprite3DVisitor = ComponentVisitor< Sprite3D >;

  struct Graphics : public System
  {
    virtual auto CreateModelComponent() -> Model* = 0 ;
    virtual void DestroyModelComponent( Model* ) = 0;
    virtual void VisitModels( ModelVisitor* ) const = 0;

    virtual auto CreateCameraComponent() -> CameraComponent* = 0 ;
    virtual void DestroyCameraComponent( CameraComponent* ) = 0;
    virtual void VisitCameras( CameraVisitor* ) const = 0;
    template<typename T> void TVisitCameras( T&& t ) const
    {
      struct : public CameraVisitor { void operator()( CameraComponent* camera ) override { (*t)( camera ); } T* t; }
      tVisitor;
      tVisitor.t = &t;
      VisitCameras( &tVisitor );
    }

    virtual auto CreateMaterialComponent() -> Material* = 0;
    virtual void DestroyMaterialComponent( Material* ) = 0;
    virtual void VisitMaterials( MaterialVisitor* ) const = 0;

    virtual auto CreateSkyboxComponent() -> Skybox* = 0;
    virtual void DestroySkyboxComponent( Skybox* ) = 0;
    virtual void VisitSkyboxes( SkyboxVisitor* ) const = 0;

    virtual auto CreateLightComponent() -> Light* = 0;
    virtual void DestroyLightComponent( Light* ) = 0;
    virtual void VisitLights( LightVisitor* ) const = 0;
    template<typename T> void TVisitLights( T&& t ) const
    {
      struct : public LightVisitor { void operator()( Light* light ) override { (*t)( light ); } T* t; }
      tVisitor;
      tVisitor.t = &t;
      VisitLights( &tVisitor );
    }

    virtual auto CreateSprite3DComponent() -> Sprite3D* = 0;
    virtual void DestroySprite3DComponent( Sprite3D* ) = 0;
    virtual void VisitSprite3Ds( Sprite3DVisitor* ) const = 0;
    template<typename T> void TVisitSprite3Ds( T&& t ) const
    {
      struct : public Sprite3DVisitor { void operator()( Sprite3D* sprite ) override { (*t)( sprite ); } T* t; }
      tVisitor;
      tVisitor.t = &t;
      VisitSprite3Ds( &tVisitor );
    }

    void Update() override;
    void DebugImgui() override;

    static void SpaceInitGraphics();
    static auto From( dynmc World* ) -> dynmc Graphics*;
    static auto From( const World* ) -> const Graphics*;
    static void GraphicsDebugImgui( System* );
  };

} // namespace Tac

