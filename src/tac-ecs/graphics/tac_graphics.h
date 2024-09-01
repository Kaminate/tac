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

  struct Graphics : public System
  {
    virtual Model*         CreateModelComponent() = 0;
    virtual void           DestroyModelComponent( Model* ) = 0;
    virtual void           VisitModels( ModelVisitor* ) const = 0;

    virtual Material*      CreateMaterialComponent() = 0;
    virtual void           DestroyMaterialComponent( Material* ) = 0;

    // any reason this should exist?
    virtual void           VisitMaterials( MaterialVisitor* ) const = 0;

    virtual Skybox*        CreateSkyboxComponent() = 0;
    virtual void           DestroySkyboxComponent( Skybox* ) = 0;
    virtual void           VisitSkyboxes( SkyboxVisitor* ) const = 0;

    virtual Light*         CreateLightComponent() = 0;
    virtual void           DestroyLightComponent( Light* ) = 0;
    virtual void           VisitLights( LightVisitor* ) const = 0;

    void                   DebugImgui() override;

    static void            SpaceInitGraphics();
    static dynmc Graphics* From( dynmc World* );
    static const Graphics* From( const World* );
  };

  void              GraphicsDebugImgui( System* );
} // namespace Tac

