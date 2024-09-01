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
//#include "tac-std-lib/meta/tac_meta_composite.h"
//#include "tac-std-lib/meta/tac_meta_fn.h"
//#include "tac-std-lib/meta/tac_meta_fn_sig.h"
//#include "tac-std-lib/meta/tac_meta_var.h"
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
  struct CreationAppState : public App::IState
  {
    CreationSimState mSimState;
  };


  static String CreationGetNewEntityName( World* world )
  {
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
    bool saved = PrefabSave( world, saveErrors );

    const TimestampDifference errorDurationSecs{ 60.0f };
    const TimestampDifference successDurationSecs{ 5.0f };
    String msg;
    if( saveErrors )
      msg = saveErrors.ToString();
    else if( saved )
      msg = "Saved Prefabs!";
    else
      msg = "Didn't save prefabs";
    const TimestampDifference duration{ saveErrors ? errorDurationSecs : successDurationSecs };
    CreationGameWindow::SetStatusMessage( msg, duration );
  }

  static void CloseAppWhenAllWindowsClosed()
  {
    const bool isAnyWindowShown{
      CreationSystemWindow::sShowWindow ||
      CreationMainWindow::sShowWindow ||
      CreationAssetView::sShowWindow ||
      CreationGameWindow::sShowWindow ||
      CreationPropertyWindow::sShowWindow };
    static bool hasAnyWindowShown;
    hasAnyWindowShown |= isAnyWindowShown;
    if( hasAnyWindowShown && !isAnyWindowShown )
      OS::OSAppStopRunning();
  }

  //===-------------- App -------------===//

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& cfg ) : App( cfg ) {}

    void Init( App::InitParams initParams, Errors& errors ) override
    {
      SpaceInit();
      gCreation.Init( mSettingsNode, errors );
    }

    IState* GetGameState() override
    {
      CreationAppState* state{ TAC_NEW CreationAppState };
      state->mSimState.CopyFrom( gCreation.mSimState );
      return state;
    }

    void Update( App::UpdateParams, Errors& errors ) override
    {
      World* world{ gCreation.mSimState.mWorld };
      Camera* camera{ gCreation.mSimState.mEditorCamera };
      gCreation.Update( world, camera,errors );
    }

    void Render( App::RenderParams renderParams, Errors& errors ) override
    {
      // todo: interpolate between old and new state?
      CreationAppState* state{ ( CreationAppState* )renderParams.mNewState };
      gCreation.Render( state, errors );
    }

    void Uninit( Errors& errors ) override
    {
      gCreation.mSimState.Uninit();
      gCreation.mSysState.Uninit();
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

    CreationMainWindow::sShowWindow =
      settingsNode.GetChild( "show main window" ).GetValueWithFallback( true );
    CreationPropertyWindow::sShowWindow =
      settingsNode.GetChild( "show property window" ).GetValueWithFallback( true );
    CreationGameWindow::sShowWindow =
      settingsNode.GetChild( "show game window" ).GetValueWithFallback( true );

    //CreationPropertyWindow::sShowWindow = true;

    sIconRenderer.Init( errors );
    mMousePicking.Init( &mSelectedEntities, &mGizmoMgr, errors );
    sWidgetRenderer.Init( &mMousePicking, &mGizmoMgr,  errors );
    mSelectedEntities.Init( mSettingsNode );
    TAC_CALL( mSimState.Init( errors ) );
    TAC_CALL( mSysState.Init( &sIconRenderer, &sWidgetRenderer, errors ) );
    TAC_CALL( mGizmoMgr.Init( &mSelectedEntities, errors ) );

    //CreationSystemWindow::Init();
    //CreationAssetView::Init();
    //CreationMainWindow::Init();
    CreationGameWindow::Init( &mGizmoMgr, &mMousePicking, mSettingsNode, errors );
    //CreationPropertyWindow::Init();
    //CreationProfileWindow::Init();

    TAC_CALL( PrefabLoad( mSettingsNode,
                          &mEntityUUIDCounter,
                          mSimState.mWorld,
                          mSimState.mEditorCamera,
                          errors ) );
  }

  void                Creation::Uninit( Errors& errors )
  {
    sIconRenderer.Uninit();
    sWidgetRenderer.Uninit();
    mSimState.Uninit();
    mSysState.Uninit();
    mGizmoMgr.Uninit();
  }

  void                Creation::Render( const CreationAppState* renderParams,
                                        Errors& errors )
  {
    World* world{ renderParams->mSimState.mWorld };
    Camera* camera{ renderParams->mSimState.mEditorCamera };

    CreationAssetView::Render( errors );
    CreationGameWindow::Render( world, camera, errors );
  }

  void                Creation::Update( World* world, Camera* camera, Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    CheckSavePrefab( world );

    world->mDebug3DDrawData->Clear();

    CreationSystemWindow::Update( world, mSettingsNode );
    TAC_CALL( CreationAssetView::Update( world, camera, errors ) );
    TAC_CALL( CreationMainWindow::Update( world, errors ) );
    TAC_CALL( CreationGameWindow::Update( world, camera, errors ) );
    TAC_CALL( CreationPropertyWindow::Update( world, camera, mSettingsNode, errors ) );
    TAC_CALL( CreationProfileWindow::Update( SimKeyboardApi(), errors ) );

    world->Step( TAC_DELTA_FRAME_SECONDS );


    mSelectedEntities.DeleteEntitiesCheck();

    CloseAppWhenAllWindowsClosed();
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

      const MetaType::CopyParams copyParams
      {
        .mDst{ copyComponent },
        .mSrc{ prefabComponent },
      };
      entry->mMetaType->Copy( copyParams );

      TAC_ASSERT_UNIMPLEMENTED; // TODO: test if the copy worked (in watch window)

      //Json entityJson;
      //entry->mSaveFn( entityJson, prefabComponent );
      //entry->mLoadFn( entityJson, copyComponent );
    }

    for( Entity* prefabChildEntity : prefabEntity->mChildren )
    {
      Entity* copyChildEntity{
        InstantiateAsCopy(
          world,
          camera,
          prefabChildEntity,
          prefabChildEntity->mRelativeSpace ) };
      copyEntity->AddChild( copyChildEntity );
    }

    return copyEntity;
  }

  Entity* Creation::CreateEntity( World* world, Camera* camera )
  {
    Entity* entity{ world->SpawnEntity( mEntityUUIDCounter.AllocateNewUUID() ) };
    entity->mName = CreationGetNewEntityName( world );
    entity->mRelativeSpace = GetEditorCameraVisibleRelativeSpace( camera );

    mSelectedEntities.Select( entity );

    return entity;
  }



} // namespace Tac

