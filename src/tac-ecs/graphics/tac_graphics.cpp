#include "tac_graphics.h" // self-inc

#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/containers/tac_set.h"

#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/camera/tac_camera_component.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/sprite3d/tac_sprite3d.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"
#include "tac-ecs/component/tac_component.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"

namespace Tac
{

  struct GraphicsImpl : public Graphics
  {
    // TODO: this shit is rediculous, i need a better ECS

    auto CreateModelComponent() -> Model* override
    {
      auto model { TAC_NEW Model };
      mModels.insert( model );
      return model;
    }
    void DestroyModelComponent( Model* model ) override
    {
      mModels.erase( model );
      TAC_DELETE model;
    }
    void VisitModels( ModelVisitor* modelVisitor ) const override
    {
      for( Model* model : mModels )
        if( model->mEntity->mActive )
          ( *modelVisitor )( model );
    }
    
    auto CreateMaterialComponent() -> Material* override
    {
      auto material { TAC_NEW Material };
      mMaterials.insert( material );
      return material;
    }
    void DestroyMaterialComponent( Material* material ) override
    {
      mMaterials.erase( material );
      TAC_DELETE material;
    }
    void VisitMaterials( MaterialVisitor* materialVisitor) const override
    {
      for( Material* material : mMaterials )
        if( material->mEntity->mActive )
          ( *materialVisitor )( material );
    }

    auto CreateSkyboxComponent() -> Skybox* override
    {
      auto skybox { TAC_NEW Skybox };
      mSkyboxes.insert( skybox );
      return skybox;
    }
    void DestroySkyboxComponent( Skybox* skybox ) override
    {
      mSkyboxes.erase( skybox );
      TAC_DELETE skybox;
    }
    void VisitSkyboxes( SkyboxVisitor* skyboxVisitor ) const override
    {
      for( Skybox* skybox : mSkyboxes )
        if( skybox->mEntity->mActive )
          ( *skyboxVisitor )( skybox );
    }

    auto CreateLightComponent() -> Light* override
    {
      auto light { TAC_NEW Light };
      mLights.insert( light );
      return light;
    }
    void DestroyLightComponent( Light* light ) override
    {
       mLights.erase( light );
      TAC_DELETE light;
    }
    void VisitLights( LightVisitor* lightVisitor ) const override
    {
      for( Light* light : mLights )
        if( light->mEntity->mActive )
          ( *lightVisitor )( light );
    }


    auto CreateCameraComponent() -> CameraComponent* override
    {
      auto camera { TAC_NEW CameraComponent };
      mCameras.insert( camera );
      return camera;
    }
    void DestroyCameraComponent( CameraComponent* camera ) override
    {
      mCameras.erase( camera );
      TAC_DELETE camera;
    }
    void VisitCameras( CameraVisitor* visitor ) const override
    {
      for( CameraComponent* cam : mCameras )
        if( cam->mEntity->mActive )
          ( *visitor )( cam );
    }

    auto CreateSprite3DComponent() -> Sprite3D* override
    {
      auto sprite{ TAC_NEW Sprite3D };
      mSprite3Ds.insert( sprite );
      return sprite;
    }
    void DestroySprite3DComponent( Sprite3D* Sprite3D ) override
    {
      mSprite3Ds.erase( Sprite3D );
      TAC_DELETE Sprite3D;
    }
    void VisitSprite3Ds( Sprite3DVisitor* visitor ) const override
    {
      for( Sprite3D* sprite : mSprite3Ds )
        if( sprite->mEntity->mActive )
          ( *visitor )( sprite );
    }



    Set< Model* >           mModels;
    Set< Skybox* >          mSkyboxes;
    Set< Light* >           mLights;
    Set< Material* >        mMaterials;
    Set< CameraComponent* > mCameras;
    Set< Sprite3D* >        mSprite3Ds;
  };

  static SystemInfo* sGraphicsInfo;

  static auto CreateGraphicsSystem() -> System* { return TAC_NEW GraphicsImpl; }

  void Graphics::Update()
  {
    for( CameraComponent* cameraComponent : ( ( GraphicsImpl* )this )->mCameras )
    {
      //dynmc Camera& camera{ cameraComponent->mCamera };
      cameraComponent->mCamera.mPos = cameraComponent->mEntity->mWorldTransform.GetColumn(3).xyz();
      cameraComponent->mCamera.mForwards = cameraComponent->mEntity->mWorldTransform.GetColumn( 2 ).xyz();
      cameraComponent->mCamera.mRight = cameraComponent->mEntity->mWorldTransform.GetColumn( 0 ).xyz();
      cameraComponent->mCamera.mUp = cameraComponent->mEntity->mWorldTransform.GetColumn( 1 ).xyz();
    }
  }

  // Why does Graphics::DebugImgui() and GraphicsDebugImgui() exist
  void Graphics::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "Graphics" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::Text( "Debug draw vert count: %i", mDebugDrawVerts.size() );

    ImGuiText( __FUNCTION__ );
    ImGuiText( "Graphics::DebugImgui()" );
  }

  void Graphics::GraphicsDebugImgui( System* system )
  {
    Graphics* graphics { ( Graphics* )system };

  #if TAC_GAME_PRESENTATION_ENABLED()
    // ??????
    GamePresentation::DebugImGui( graphics );
  #endif

  }


  void Graphics::SpaceInitGraphics()
  {
    sGraphicsInfo = SystemInfo::Register();
    sGraphicsInfo->mName = "Graphics";
    sGraphicsInfo->mCreateFn = CreateGraphicsSystem;
    sGraphicsInfo->mDebugImGui = GraphicsDebugImgui;

    Model::RegisterComponent();
    Skybox::RegisterComponent();
    Light::RegisterComponent();
    Material::RegisterComponent();
    CameraComponent::RegisterComponent();
    Sprite3D::RegisterComponent();
  }

  auto Graphics::From( dynmc World* world ) -> dynmc Graphics* { return ( dynmc Graphics* )world->GetSystem( sGraphicsInfo ); }
  auto Graphics::From( const World* world ) -> const Graphics* { return ( const Graphics* )world->GetSystem( sGraphicsInfo ); }

} // namespace Tac

