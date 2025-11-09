#include "tac_level_editor.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/tac_space.h"
#include "tac-ecs/terrain/tac_terrain.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
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


namespace Tac
{
  static auto CreationGetNewEntityName( World* world ) -> String
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

  static void CheckSavePrefab()
  {
    if( !Creation::IsGameRunning() &&
        AppKeyboardApi::IsPressed( Key::Modifier ) &&
        AppKeyboardApi::JustPressed( Key::S ) )
    {
      dynmc Errors saveErrors;
      if( PrefabSave( &Creation::GetData()->mWorld, saveErrors ) )
        CreationGameWindow::SetStatusMessage( "Saved Prefabs!", TimestampDifference { 5.f } );
      if( saveErrors)
        CreationGameWindow::SetStatusMessage( saveErrors.ToString(), TimestampDifference { 60 } );
    }
  }

  static void CloseAppWhenAllWindowsClosed()
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

    void Load() { for( Data& data : mDatas ) Load( data.mWindowName, *data.mShow ); } 
    void Save() { for( Data& data : mDatas ) Save( data.mWindowName, *data.mShow ); } 

  private:
    struct Data
    {
      StringView mWindowName {};
      bool*      mShow       {};
    };

    ShowWindowHelper()
    {
      AddData( "main", &CreationMainWindow::sShowWindow );
      AddData( "property", &CreationPropertyWindow::sShowWindow );
      AddData( "game", &CreationGameWindow::sShowWindow );
      AddData( "asset", &CreationAssetView::sShowWindow );
    }

    void Load( StringView windowName, bool& show )
    {
      const ShortFixedString path{ MakePath( windowName ) };
      show = Shell::sShellSettings.GetChild( path ).GetValueWithFallback( true );
    }

    void Save( StringView windowName, bool& show )
    {
      const ShortFixedString path{ MakePath( windowName ) };
      Shell::sShellSettings.GetChild( path ).SetValue( show );
    }

    void AddData( StringView windowName, bool* show )
    {
      Data data;
      data.mWindowName = windowName;
      data.mShow = show;
      mDatas.push_back( data );
    }

    auto MakePath( StringView name ) -> ShortFixedString
    {
      return ShortFixedString::Concat( "show window.", name );
    }

    Vector< Data > mDatas;
  };


  //===-------------- Creation -------------===//

  static bool           sIsGameRunning {};
  static Creation::Data sGameStuff     {}; // game camera should be controlled through game script
  static Creation::Data sEditorStuff   {};

  void Creation::Init( Errors& errors )
  {
    ShowWindowHelper::GetInstance().Load();
    IconRenderer::Init( errors );
    CreationMousePicking::sInstance.Init( errors );
    WidgetRenderer::Init( errors );
    sGameStuff.mWorld.Init();
    sEditorStuff.mWorld.Init();
    sEditorStuff.mCamera = Camera
    {
      .mPos       { 0, 1, 5 },
      .mForwards  { 0, 0, -1 },
      .mRight     { 1, 0, 0 },
      .mUp        { 0, 1, 0 }
    };
    TAC_CALL( GamePresentation::Init( errors ) );
    CreationGameWindow::Init( errors );
    TAC_CALL( PrefabLoad( errors ) );
  }

  void Creation::Uninit( Errors& )
  {
    ShowWindowHelper::GetInstance().Save();
    IconRenderer::Uninit();
    WidgetRenderer::Uninit();
    GamePresentation::Uninit(); 
  }

  void Creation::Render( World* world, Camera* camera, Errors& errors )
  {
    CreationAssetView::Render( errors );
    CreationGameWindow::Render( world, camera, errors );
  }

  void Creation::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;
    CheckSavePrefab();

    for( Data* datas[]{ &sEditorStuff, &sGameStuff }; Data* data : datas )
        data->mWorld.mDebug3DDrawData->Clear();

    // Update the main window first so it becomes the parent window (maybe)
    TAC_CALL( CreationMainWindow::Update( errors ) );
    CreationSystemWindow::Update();
    TAC_CALL( CreationAssetView::Update( errors ) );
    TAC_CALL( CreationShaderGraphWindow::Update(  errors ) );
    TAC_CALL( CreationGameWindow::Update( errors ) );
    TAC_CALL( CreationPropertyWindow::Update( errors ) );
    TAC_CALL( CreationProfileWindow::Update( errors ) );

    if( sIsGameRunning )
      sGameStuff.mWorld.Step( TAC_DELTA_FRAME_SECONDS );

    SelectedEntities::DeleteEntitiesCheck();
    CloseAppWhenAllWindowsClosed();
  }

  auto Creation::GetEditorCameraVisibleRelativeSpace( const Camera* camera ) -> RelativeSpace
  {
    return RelativeSpace
    {
      .mPosition { camera->mPos + camera->mForwards * 5.0f },
    };
  }

  static auto InstantiateAsCopyAux( Entity* prefabEntity, const RelativeSpace& relativeSpace ) -> Entity*
  {
    Entity* copyEntity{ Creation::CreateEntity() };
    copyEntity->mRelativeSpace = relativeSpace;
    copyEntity->mInheritParentScale = prefabEntity->mInheritParentScale;
    copyEntity->mName = prefabEntity->mName;

    for( Component* prefabComponent : prefabEntity->mComponents )
    {
      const ComponentInfo* entry{ prefabComponent->GetEntry() };
      dynmc Component* copyComponent{ copyEntity->AddNewComponent( prefabComponent->GetEntry() ) };
      entry->mMetaType->Copy(
        MetaType::CopyParams
        {
          .mDst { copyComponent },
          .mSrc { prefabComponent },
        } );
    }
                              
    for( Entity* prefabChildEntity : prefabEntity->mChildren )
      if( Entity * copyChildEntity{
        InstantiateAsCopyAux( prefabChildEntity, prefabChildEntity->mRelativeSpace ) } )
        copyEntity->AddChild( copyChildEntity );

    return copyEntity;
  }

  auto Creation::InstantiateAsCopy( Entity* prefabEntity ) -> Entity*
  {
    const RelativeSpace relativeSpace{ GetEditorCameraVisibleRelativeSpace( GetCamera() ) };
    return InstantiateAsCopyAux( prefabEntity, relativeSpace );
  }

  auto Creation::CreateEntity() -> Entity*
  {
    Data* data{ GetData() };
    Entity* entity{ data->mWorld.SpawnEntity( data->mEntityUUIDCounter.AllocateNewUUID() ) };
    entity->mName = CreationGetNewEntityName( &data->mWorld );
    entity->mRelativeSpace = GetEditorCameraVisibleRelativeSpace( &data->mCamera );
    SelectedEntities::Select( entity );
    return entity;
  }


  auto Creation::GetData() -> Data*     { return sIsGameRunning ? &sGameStuff : &sEditorStuff; }
  auto Creation::GetCamera() -> Camera* { return &GetData()->mCamera; }
  auto Creation::GetWorld() -> World*   { return &GetData()->mWorld; }
  bool Creation::IsGameRunning()        { return sIsGameRunning; }

} // namespace Tac

