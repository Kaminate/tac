#pragma once

#include "tac-ecs/system/tac_system.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{

  template< typename T > struct ComponentVisitor { virtual void operator()( T* ) = 0; };
  typedef ComponentVisitor< Model > ModelVisitor;
  typedef ComponentVisitor< Skybox > SkyboxVisitor;
  typedef ComponentVisitor< Light > LightVisitor;

  struct Graphics : public System
  {
    virtual Model*  CreateModelComponent() = 0;
    virtual void    DestroyModelComponent( Model* ) = 0;
    virtual void    VisitModels( ModelVisitor* ) const = 0;

    virtual Skybox* CreateSkyboxComponent() = 0;
    virtual void    DestroySkyboxComponent( Skybox* ) = 0;
    virtual void    VisitSkyboxes( SkyboxVisitor* ) const = 0;

    virtual Light*  CreateLightComponent() = 0;
    virtual void    DestroyLightComponent( Light* ) = 0;
    virtual void    VisitLights( LightVisitor* ) const = 0;

    void            DebugImgui() override;
    static void     SpaceInitGraphics();
  };

  Graphics*         GetGraphics( World* );
  const Graphics*   GetGraphics( const World* );
  void              GraphicsDebugImgui( System* );
}

