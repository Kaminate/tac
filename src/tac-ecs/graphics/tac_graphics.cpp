#include "tac_graphics.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_set.h"

#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/world/tac_world.h"

namespace Tac
{

  struct GraphicsImpl : public Graphics
  {

    // TODO: this shit is rediculous, i need a better ECS

    //                  Model

    Model*              CreateModelComponent() override
    {
      auto model { TAC_NEW Model };
      mModels.insert( model );
      return model;
    }

    void                DestroyModelComponent( Model* model ) override
    {
      mModels.erase( model );
      TAC_DELETE model;
    }

    void                VisitModels( ModelVisitor* modelVisitor ) const override
    {
      for( Model* model : mModels )
        if( model->mEntity->mActive )
          ( *modelVisitor )( model );
    }
    
    //                  Material
    Material*   CreateMaterialComponent() override
    {
      auto material { TAC_NEW Material };
      mMaterials.insert( material );
      return material;
    }

    void        DestroyMaterialComponent( Material* material ) override
    {
      mMaterials.erase( material );
      TAC_DELETE material;
    }

    void        VisitMaterials( MaterialVisitor* materialVisitor) const override
    {
      for( Material* material : mMaterials )
        if( material->mEntity->mActive )
          ( *materialVisitor )( material );
    }


    //                  Skybox

    Skybox*             CreateSkyboxComponent() override
    {
      auto skybox { TAC_NEW Skybox };
      mSkyboxes.insert( skybox );
      return skybox;
    }

    void                DestroySkyboxComponent( Skybox* skybox ) override
    {
      mSkyboxes.erase( skybox );
      TAC_DELETE skybox;
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
      auto light { TAC_NEW Light };
      mLights.insert( light );
      return light;
    }

    void                DestroyLightComponent( Light* light ) override
    {
       mLights.erase( light );
      TAC_DELETE light;
    }

    void                VisitLights( LightVisitor* lightVisitor ) const override
    {
      for( Light* light : mLights )
        if( light->mEntity->mActive )
          ( *lightVisitor )( light );
    }


    Set< Model* >     mModels;
    Set< Skybox* >    mSkyboxes;
    Set< Light* >     mLights;
    Set< Material* >  mMaterials;
  };

  static SystemInfo* sGraphicsInfo;

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


  void              Graphics::SpaceInitGraphics()
  {
    sGraphicsInfo = SystemInfo::Register();
    sGraphicsInfo->mName = "Graphics";
    sGraphicsInfo->mCreateFn = CreateGraphicsSystem;
    sGraphicsInfo->mDebugImGui = GraphicsDebugImgui;

    Model::RegisterComponent();
    Skybox::RegisterComponent();
    Light::RegisterComponent();
    Material::RegisterComponent();
  }

  dynmc Graphics*   Graphics::From( dynmc World* world ) { return ( dynmc Graphics* )world->GetSystem( sGraphicsInfo ); }
  const Graphics*   Graphics::From( const World* world ) { return ( const Graphics* )world->GetSystem( sGraphicsInfo ); }

} // namespace Tac

