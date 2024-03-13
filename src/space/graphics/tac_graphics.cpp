#include "tac_graphics.h" // self-inc

#include "tac-rhi/ui/imgui/tac_imgui.h"
#include "tac-rhi/ui/tac_font.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_set.h"

#include "space/graphics/light/tac_light.h"
#include "space/graphics/model/tac_model.h"
#include "space/ecs/tac_entity.h"
#include "space/graphics/skybox/tac_skybox_component.h"
#include "space/ecs/tac_component.h"
#include "space/world/tac_world.h"

namespace Tac
{

  struct GraphicsImpl : public Graphics
  {

    // TODO: this shit is rediculous, i need a better ECS

    //                  Model

    Model*              CreateModelComponent() override
    {
      auto model = TAC_NEW Model;
      mModels.insert( model );
      return model;
    }

    void                DestroyModelComponent( Model* model ) override
    {
      mModels.erase( model );
      delete model;
    }

    void                VisitModels( ModelVisitor* modelVisitor ) const override
    {
      for( Model* model : mModels )
        if( model->mEntity->mActive )
          ( *modelVisitor )( model );
    }


    //                  Skybox

    Skybox*             CreateSkyboxComponent() override
    {
      auto skybox = TAC_NEW Skybox;
      mSkyboxes.insert( skybox );
      return skybox;
    }

    void                DestroySkyboxComponent( Skybox* skybox ) override
    {
      mSkyboxes.erase( skybox );
      delete skybox;
    }

    void                VisitSkyboxes( SkyboxVisitor* skyboxVisitor ) const override
    {
      for( Skybox* skybox : mSkyboxes )
        if( skybox->mEntity->mActive )
          ( *skyboxVisitor )( skybox );
    }

    // Light

    Light*              CreateLightComponent() override
    {
      auto light = TAC_NEW Light;
      mLights.insert( light );
      return light;
    }

    void                DestroyLightComponent( Light* light ) override
    {
       mLights.erase( light );
      delete light;
    }

    void                VisitLights( LightVisitor* lightVisitor ) const override
    {
      for( Light* light : mLights )
        if( light->mEntity->mActive )
          ( *lightVisitor )( light );
    }


    Set< Model* >  mModels;
    Set< Skybox* > mSkyboxes;
    Set< Light* >  mLights;
  };

  static SystemRegistryEntry* gGraphicsSystemRegistryEntry;

  static System* CreateGraphicsSystem()
  {
    return TAC_NEW GraphicsImpl;
  }

  // Why does Graphics::DebugImgui() and GraphicsDebugImgui() exist
  void      Graphics::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "Graphics" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::Text( "Debug draw vert count: %i", mDebugDrawVerts.size() );

    ImGuiText( __FUNCTION__ );
    ImGuiText( "Graphics::DebugImgui()" );
  }

  void      GraphicsDebugImgui( System* ); // Defined in tacgraphicsdebug.cpp

  void      Graphics::SpaceInitGraphics()
  {
    gGraphicsSystemRegistryEntry = SystemRegisterNewEntry();
    gGraphicsSystemRegistryEntry->mCreateFn = CreateGraphicsSystem;
    gGraphicsSystemRegistryEntry->mName = "Graphics";
    gGraphicsSystemRegistryEntry->mDebugImGui = GraphicsDebugImgui;
    RegisterModelComponent();
    RegisterSkyboxComponent();
    RegisterLightComponent();
  }

  Graphics* GetGraphics( World* world )
  {
    return ( Graphics* )world->GetSystem( gGraphicsSystemRegistryEntry );
  }

  const Graphics*   GetGraphics( const World* world )
  {
    return ( const Graphics* )world->GetSystem( gGraphicsSystemRegistryEntry );
  }

}

