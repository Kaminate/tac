#pragma once

//#include "src/common/graphics/tacRenderer.h"
//#include "src/common/tacLocalization.h"
//#include "src/common/containers/tacVector.h"
//#include "src/space/model/tacModel.h"
#include "src/space/tacSystem.h"


namespace Tac
{
  //struct FontStuff;

  struct Model;
  struct Skybox;
  struct World;

  template< typename T > struct ComponentVisitor { virtual void operator()( T* ) = 0; };
  typedef ComponentVisitor< Model > ModelVisitor;
  typedef ComponentVisitor< Skybox > SkyboxVisitor;

  //struct ModelVisitor { virtual void operator()( Model* ) = 0; };
  //struct ModelVisitor { virtual void operator()( Model* ) = 0; };
  //struct Component;
  //struct ComponentVisitor { virtual void operator()( Component* ) = 0; };

  struct Graphics : public System
  {
    //typedef void    ModelVisitor( Model* );
    virtual Model*  CreateModelComponent() = 0;
    virtual void    DestroyModelComponent( Model* ) = 0;
    virtual void    VisitModels( ModelVisitor* ) = 0;

    //typedef void    SkyboxVisitor( Skybox* );
    virtual Skybox* CreateSkyboxComponent() = 0;
    virtual void    DestroySkyboxComponent( Skybox* ) = 0;
    virtual void    VisitSkyboxes( SkyboxVisitor* ) = 0;

    void            DebugImgui() override;
  };

  void              RegisterGraphicsSystem();
  Graphics*         GetGraphics( World* );
}

