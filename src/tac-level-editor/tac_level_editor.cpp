#include "tac_level_editor.h" // self-inc

#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta_fn.h"
#include "tac-std-lib/meta/tac_meta_fn_sig.h"
#include "tac-std-lib/meta/tac_meta_var.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"

// level_editor
#include "tac-level-editor/tac_level_editor_asset_view.h"
#include "tac-level-editor/tac_level_editor_game_window.h"
#include "tac-level-editor/tac_level_editor_main_window.h"
#include "tac-level-editor/tac_level_editor_prefab.h"
#include "tac-level-editor/tac_level_editor_profile_window.h"
#include "tac-level-editor/tac_level_editor_property_window.h"
#include "tac-level-editor/tac_level_editor_system_window.h"

// shell
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

// space
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/presentation/tac_shadow_presentation.h"
#include "tac-ecs/presentation/tac_skybox_presentation.h"
#include "tac-ecs/presentation/tac_voxel_gi_presentation.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/tac_space.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/terrain/tac_terrain.h"

#include "tac-level-editor/tac_level_editor_icon_renderer.h"
#include "tac-level-editor/tac_level_editor_widget_renderer.h"

Tac::Creation Tac::gCreation;

namespace Tac
{
  static const TimestampDifference errorDurationSecs { 60.0f };
  static const TimestampDifference successDurationSecs { 5.0f };

  static IconRenderer     sIconRenderer{};
  static WidgetRenderer   sWidgetRenderer{};

  static String CreationGetNewEntityName( World* world )
  {
    World* world { world };
    String desiredEntityName { "Entity" };
    int parenNumber { 1 };
    for( ;; )
    {
      if( !world->FindEntity( desiredEntityName ) )
        break;

      desiredEntityName = "Entity (" + ToString( parenNumber ) + ")";
      parenNumber++;
    }

    return desiredEntityName;
  }

  static void   CheckSavePrefab(World* world)
  {

    SimKeyboardApi keyboardApi;

    const bool triggered{
      keyboardApi.JustPressed( Key::S ) &&
      keyboardApi.IsPressed( Key::Modifier ) };

    if( !triggered )
      return;

    Errors saveErrors;
    PrefabSave( world, saveErrors );

    CreationGameWindow* window { CreationGameWindow::Instance };
    if( window )
    {
      if( saveErrors )
      {
        window->SetStatusMessage( saveErrors.ToString(), errorDurationSecs );
      }
      else
      {
        window->SetStatusMessage( "Saved prefabs!", successDurationSecs );
      }
    }
  }

  //===-------------- App -------------===//

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& cfg ) : App( cfg ) {}

    void Init( App::InitParams initParams, Errors& errors ) override
    {
      sIconRenderer.Init( errors );
      sWidgetRenderer.Init( errors );
      gCreation.mSimState.Init(errors);
      gCreation.mSysState.Init( &sIconRenderer, &sWidgetRenderer, errors );
      gCreation.Init( mSettingsNode, errors );
    }

    void Update( App::UpdateParams, Errors& errors ) override
    {
      gCreation.Update( errors );
    }

    void Render( App::RenderParams, Errors& errors ) override
    {
      gCreation.Render( errors );
    }

    void Uninit( Errors& errors ) override
    {
      gCreation.Uninit( errors );
    }
  };

  App* App::Create()
  {
    const App::Config config
    {
      .mName { "Level Editor"  },
    };
    return TAC_NEW LevelEditorApp( config );
  }

  //===-------------- Creation -------------===//

  void                Creation::Init( SettingsNode settingsNode, Errors& errors )
  {
    mSettingsNode = settingsNode;

    TAC_CALL( mSimState.Init( errors ) );

    TAC_CALL( SkyboxPresentationInit( errors ) );

    TAC_CALL( GamePresentationInit( errors ) );

    TAC_CALL( ShadowPresentationInit( errors ) );

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    TAC_CALL( VoxelGIPresentationInit( errors ) );
#endif

    TAC_CALL( PrefabLoad( mSettingsNode,
                          &mEntityUUIDCounter,
                          mSimState.mWorld,
                          mSimState.mEditorCamera,
                          errors ) );

    mSelectedEntities.Init( mSettingsNode );
  }

  void                Creation::Uninit( Errors& errors )
  {
    SkyboxPresentationUninit();
    GamePresentationUninit();
#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    VoxelGIPresentationUninit();
#endif
    ShadowPresentationUninit();

  }

  void                Creation::Render( Errors& )
  {
  }

  void                Creation::Update( World* world, Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    CheckSavePrefab( world );


    if( mUpdateAssetView )
      CreationUpdateAssetView();

    world->Step( TAC_DELTA_FRAME_SECONDS );


    mSelectedEntities.DeleteEntitiesCheck();
  }


  RelativeSpace       Creation::GetEditorCameraVisibleRelativeSpace( const Camera* camera )
  {
    return RelativeSpace
    {
      .mPosition { camera->mPos + camera->mForwards * 5.0f },
    };
  }

  Entity*             Creation::InstantiateAsCopy( World* world,
                                                   Camera* camera,
                                                   Entity* prefabEntity,
                                                   const RelativeSpace& relativeSpace )
  {
    Entity* copyEntity{ CreateEntity( world, camera ) };
    copyEntity->mRelativeSpace = relativeSpace;
    copyEntity->mInheritParentScale = prefabEntity->mInheritParentScale;
    copyEntity->mName = prefabEntity->mName;

    for( Component* prefabComponent : prefabEntity->mComponents )
    {
      const ComponentRegistryEntry* entry { prefabComponent->GetEntry() };
      Component* copyComponent { copyEntity->AddNewComponent( prefabComponent->GetEntry() ) };
      Json entityJson;
      entry->mSaveFn( entityJson, prefabComponent );
      entry->mLoadFn( entityJson, copyComponent );
    }

    for( Entity* prefabChildEntity : prefabEntity->mChildren )
    {
      Entity* copyChildEntity{
        InstantiateAsCopy(
          world,
          camera,
          prefabChildEntity,
          prefabChildEntity->mRelativeSpace ) };
      //Entity* copyChildEntity = CreateEntity();
      copyEntity->AddChild( copyChildEntity );
    }

    return copyEntity;
  }

  Entity* Creation::CreateEntity( World* world, Camera* camera )
  {
    Entity* entity { world->SpawnEntity( mEntityUUIDCounter.AllocateNewUUID() ) };
    entity->mName = CreationGetNewEntityName(world);
    entity->mRelativeSpace = GetEditorCameraVisibleRelativeSpace( camera );

    mSelectedEntities.Select( entity );

    return entity;
  }



} // namespace Tac

