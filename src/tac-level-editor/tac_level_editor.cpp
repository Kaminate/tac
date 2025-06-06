#include "tac_level_editor.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/tac_space.h"
#include "tac-ecs/terrain/tac_terrain.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-level-editor/tac_level_editor_asset_view.h"
#include "tac-level-editor/tac_level_editor_game_window.h"
#include "tac-level-editor/tac_level_editor_icon_renderer.h"
#include "tac-level-editor/tac_level_editor_main_window.h"
#include "tac-level-editor/tac_level_editor_prefab.h"
#include "tac-level-editor/tac_level_editor_profile_window.h"
#include "tac-level-editor/tac_level_editor_property_window.h"
#include "tac-level-editor/tac_level_editor_shader_graph_window.h"
#include "tac-level-editor/tac_level_editor_system_window.h"
#include "tac-level-editor/tac_level_editor_widget_renderer.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"

Tac::Creation Tac::gCreation;

namespace Tac
{
  struct CreationAppState : public App::IState
  {
    CreationSimState mSimState;
  };

  static String CreationGetNewEntityName( World* world )
  {
    dynmc String desiredEntityName { "Entity" };
    dynmc int parenNumber { 1 };
    for( ;; )
    {
      if( !world->FindEntity( desiredEntityName ) )
        break;

      desiredEntityName = "Entity (" + ToString( parenNumber ) + ")";
      parenNumber++;
    }

    return desiredEntityName;
  }

  static void   CheckSavePrefab( World* world )
  {
    if( const bool triggered{
          AppKeyboardApi::JustPressed( Key::S ) &&
          AppKeyboardApi::IsPressed( Key::Modifier ) };
          !triggered )
      return;

    dynmc Errors saveErrors;
    const bool saved{ PrefabSave( world, saveErrors ) };
    const TimestampDifference errorDurationSecs{ 60.0f };
    const TimestampDifference successDurationSecs{ 5.0f };
    const String msg{ [ & ]() ->String {
      if( saveErrors ) { return saveErrors.ToString(); }
      if( saved ) { return "Saved Prefabs!"; }
      return "Didn't save prefabs";
    }( ) };
    const TimestampDifference duration{ saveErrors ? errorDurationSecs : successDurationSecs };
    CreationGameWindow::SetStatusMessage( msg, duration );
  }

  static void   CloseAppWhenAllWindowsClosed()
  {
    static bool hasAnyWindowShown;
    const bool isAnyWindowShown{
      CreationSystemWindow::sShowWindow ||
      CreationMainWindow::sShowWindow ||
      CreationAssetView::sShowWindow ||
      CreationGameWindow::sShowWindow ||
      CreationShaderGraphWindow::sShowWindow ||
      CreationPropertyWindow::sShowWindow };
    hasAnyWindowShown |= isAnyWindowShown;
    if( hasAnyWindowShown && !isAnyWindowShown )
      OS::OSAppStopRunning();
  }

  struct ShowWindowHelper
  {
  public:

    static ShowWindowHelper& GetInstance()
    {
      static ShowWindowHelper sInstance;
      return sInstance;
    }

    void Load( SettingsNode settingsNode )
    {
      for( Data& data : mDatas )
      {
        Load( settingsNode, data.mWindowName, *data.mShow );
      }
    }

    void Save( SettingsNode settingsNode )
    {
      for( Data& data : mDatas )
      {
        Save( settingsNode, data.mWindowName, *data.mShow );
      }
    }


  private:
    struct Data
    {
      StringView mWindowName;
      bool*      mShow;
    };

    ShowWindowHelper()
    {
      AddData( "main", &CreationMainWindow::sShowWindow );
      AddData( "property", &CreationPropertyWindow::sShowWindow );
      AddData( "game", &CreationGameWindow::sShowWindow );
      AddData( "asset", &CreationAssetView::sShowWindow );
    }

    void             Load(  SettingsNode settingsNode, StringView windowName, bool& show )
    {
      const ShortFixedString path{ MakePath( windowName ) };
      show = settingsNode.GetChild( path ).GetValueWithFallback( true );
    }

    void             Save(  SettingsNode settingsNode, StringView windowName, bool& show )
    {
      const ShortFixedString path{ MakePath( windowName ) };
      settingsNode.GetChild( path ).SetValue( show );
    }

    void             AddData( StringView windowName, bool* show )
    {
      Data data;
      data.mWindowName = windowName;
      data.mShow = show;
      mDatas.push_back( data );
    }

    ShortFixedString MakePath( StringView name )
    {
      return ShortFixedString::Concat( "show window.", name );
    }

    Vector< Data > mDatas;
  };


  //===-------------- App -------------===//

  struct LevelEditorApp : public App
  {
    LevelEditorApp( const Config& cfg ) : App( cfg ) {}

    void Init( Errors& errors ) override
    {
      SpaceInit();
      gCreation.Init( mSettingsNode, errors );
    }

    State GameState_Create() override
    {
      return TAC_NEW CreationAppState;
    }

    void  GameState_Update( IState* state ) override
    {
      ( ( CreationAppState* )state )->mSimState.CopyFrom( gCreation.mSimState );
    }

    void Update( Errors& errors ) override
    {
      World* world{ gCreation.mSimState.mWorld };
      Camera* camera{ gCreation.mSimState.mEditorCamera };
      gCreation.Update( world, camera, errors );
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

    CreationAppState mState;
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

    ShowWindowHelper::GetInstance().Load( settingsNode );

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
    ShowWindowHelper::GetInstance().Save( mSettingsNode );
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

    // Update the main window first so it becomes the parent window (maybe)
    TAC_CALL( CreationMainWindow::Update( world, errors ) );

    CreationSystemWindow::Update( world, mSettingsNode );
    TAC_CALL( CreationAssetView::Update( world, camera, errors ) );
    TAC_CALL( CreationShaderGraphWindow::Update(  errors ) );
    TAC_CALL( CreationGameWindow::Update( world, camera, errors ) );
    TAC_CALL( CreationPropertyWindow::Update( world, camera, mSettingsNode, errors ) );
    TAC_CALL( CreationProfileWindow::Update(  errors ) );

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
      const ComponentInfo* entry{ prefabComponent->GetEntry() };
      const MetaType* metaType{ entry->mMetaType };
      dynmc Component* copyComponent{ copyEntity->AddNewComponent( prefabComponent->GetEntry() ) };
      const MetaType::CopyParams copyParams
      {
        .mDst { copyComponent },
        .mSrc { prefabComponent },
      };
      metaType->Copy( copyParams );


      //Json entityJson;
      //entry->mSaveFn( entityJson, prefabComponent );
      //entry->mLoadFn( entityJson, copyComponent );
    }

    for( Entity* prefabChildEntity : prefabEntity->mChildren )
    {
      Entity* copyChildEntity{ InstantiateAsCopy( world,
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

