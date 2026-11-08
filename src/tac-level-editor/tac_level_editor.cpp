#include "tac_level_editor.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/camera/tac_camera_component.h"
#include "tac-ecs/tac_space.h"
#include "tac-ecs/terrain/tac_terrain.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-level-editor/prefab/tac_level_editor_prefab.h"
#include "tac-level-editor/render/tac_level_editor_icon_renderer.h"
#include "tac-level-editor/render/tac_level_editor_widget_renderer.h"
#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-level-editor/windows/assetview/tac_level_editor_asset_view.h"
#include "tac-level-editor/picking/tac_level_editor_mouse_picking.h"
#include "tac-level-editor/windows/game/tac_level_editor_game_window.h"
#include "tac-level-editor/windows/main/tac_level_editor_main_window.h"
#include "tac-level-editor/windows/profile/tac_level_editor_profile_window.h"
#include "tac-level-editor/windows/property/tac_level_editor_property_window.h"
#include "tac-level-editor/windows/shadergraph/tac_level_editor_shader_graph_window.h"
#include "tac-level-editor/windows/system/tac_level_editor_system_window.h"
#include "tac-level-editor/tools/tac_level_editor_tool.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-engine-core/os/tac_os.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"

#include "tac-std-lib/dataprocess/tac_log.h"


namespace Tac
{
  static auto CreationGetNewEntityName( World* world ) -> String
  {
    // This is what unity3d does
    const String desiredEntityPrefix { "Entity" };
    dynmc String desiredEntitySuffix { "" };
    dynmc int parenNumber { 1 };
    for( ;; )
    {
      if( !world->FindEntity( desiredEntityPrefix + desiredEntitySuffix ) )
        break;

      desiredEntitySuffix = " (" + ToString( parenNumber ) + ")";
      parenNumber++;
    }

    return desiredEntityPrefix + desiredEntitySuffix;
  }

  static void CheckSavePrefab()
  {
    if( !Creation::IsGameRunning() &&
        AppKeyboardApi::IsPressed( Key::Modifier ) &&
        AppKeyboardApi::JustPressed( Key::S ) )
    {
      dynmc Errors saveErrors;
      if( PrefabSave( Creation::GetWorld(), saveErrors ) )
        CreationGameWindow::SetStatusMessage( "Saved Prefabs!", GameTimeDelta{ .mSeconds{ 5.f } } );
      if( saveErrors)
        CreationGameWindow::SetStatusMessage( saveErrors.ToString(), GameTimeDelta{ .mSeconds{  60 } } );
    }
  }

  static void CloseAppWhenAllWindowsClosed()
  {
    StringView systemName = CreationSystemWindow::GetWindowName();
    StringView mainName = CreationMainWindow::GetWindowName();
    StringView assetName = CreationAssetView::GetWindowName();
    StringView propertyName = CreationPropertyWindow::GetWindowName();
    StringView shaderGraphName = CreationShaderGraphWindow::GetWindowName();
    StringView gameName = CreationGameWindow::GetWindowName();

    Array names
    {
      systemName ,
      mainName ,
      assetName ,
      propertyName ,
      shaderGraphName ,
      gameName ,
    };

    int numShown = 0;
    for( StringView name : names )
    {
      WindowHandle windowHandle = ImGuiGetWindowHandle( name );
      if( windowHandle.IsValid() )
      {
        bool shown = AppWindowApi::IsShown( windowHandle );
        if( shown )
          numShown++;
      }
    }

    /*
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
      */

    static bool anyEverShown = false;
    anyEverShown |= numShown > 0;
    if( anyEverShown && !numShown)
      OS::OSAppStopRunning();
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

  struct ShowWindowHelper
  {
  public:

    static ShowWindowHelper& GetInstance()
    {
      static ShowWindowHelper sInstance;
      return sInstance;
    }

    void Load()
    {
      for( Data& data : mDatas )
        Load( data );
    } 
    void Save()
    {
      for( Data& data : mDatas )
        Save( data );
    } 

  private:
    struct Data
    {
      String mWindowName_savefile {};
      String mWindowName_wndmgr {};
      bool* mShow       {};
    };

    ShowWindowHelper()
    {
      mDatas.push_back( Data 
      {
        .mWindowName_savefile { "main" },
        .mWindowName_wndmgr   { CreationMainWindow::GetWindowName()},
        .mShow                { &CreationMainWindow::sShowWindow},
      } );

      mDatas.push_back( Data 
      {
        .mWindowName_savefile { "property" },
        .mWindowName_wndmgr   { CreationPropertyWindow::GetWindowName()},
        .mShow                { &CreationPropertyWindow::sShowWindow},
      } );
      mDatas.push_back( Data 
      {
        .mWindowName_savefile { "game" },
        .mWindowName_wndmgr   { CreationGameWindow::GetWindowName()},
        .mShow                { &CreationGameWindow::sShowWindow},
      } );
      mDatas.push_back( Data 
      {
        .mWindowName_savefile { "asset" },
        .mWindowName_wndmgr   { CreationAssetView::GetWindowName()},
        .mShow                { &CreationAssetView::sShowWindow},
      } );
      mDatas.push_back( Data 
      {
        .mWindowName_savefile { "system" },
        .mWindowName_wndmgr   { CreationSystemWindow::GetWindowName()},
        .mShow                { &CreationSystemWindow::sShowWindow},
      } );
      mDatas.push_back( Data 
      {
        .mWindowName_savefile { "shadergraph" },
        .mWindowName_wndmgr   { CreationShaderGraphWindow::GetWindowName()},
        .mShow                { &CreationShaderGraphWindow::sShowWindow},
      } );
    }

    void Load( Data& data )
    {
      const ShortFixedString path{ MakePath( data.mWindowName_savefile ) };
      *data.mShow = Shell::sShellSettings.GetChild( path ).GetValueWithFallback( true );
    }

    void Save(Data&data)
    {
      const ShortFixedString path{ MakePath( data.mWindowName_savefile  ) };
      Shell::sShellSettings.GetChild( path ).SetValue( *data.mShow );
    }

    auto MakePath( StringView name ) -> ShortFixedString
    {
      return ShortFixedString::Concat( "show window.", name );
    }

    Vector< Data > mDatas;
  };

  //===-------------- Creation -------------===//

  static bool                 sIsGameRunning           {};
  static Camera               sEditorCamera            {};
  static World                sEditorWorld             {};
  static EntityUUIDCounter    sEditorEntityUUIDCounter {};
  static World                sGameWorld               {};
  static EntityUUIDCounter    sGameEntityUUIDCounter   {};
  static StringView           sShowOnlyWindowName      {};

  void Creation::Init( Errors& errors )
  {
    ShowWindowHelper::GetInstance().Load();
    TAC_CALL(IconRenderer::Init( errors ));
    TAC_CALL(CreationMousePicking::Init( errors ));
    TAC_CALL(WidgetRenderer::Init( errors ));
    Toolbox::Init();
    sGameWorld.Init();
    sEditorWorld.Init();
    sEditorCamera = Camera
    {
      .mPos       { 0, 1, 5 },
      .mForwards  { 0, 0, -1 },
      .mRight     { 1, 0, 0 },
      .mUp        { 0, 1, 0 }
    };
    TAC_CALL( GamePresentation::Init( errors ) );
    TAC_CALL( PrefabLoad( errors ) );
    TAC_CALL( CreationGameWindow::Init( errors ));
  }

  void Creation::Uninit( Errors& )
  {

    LogApi::LogHACK_HACK_HACK____STOP_LOGGING();

    ShowWindowHelper::GetInstance().Save();
    IconRenderer::Uninit();
    WidgetRenderer::Uninit();
    GamePresentation::Uninit(); 
  }

  void Creation::Render( World* world, const Camera* camera, Errors& errors )
  {
    CreationAssetView::Render( errors );
    CreationGameWindow::Render( world, camera, errors );
  }

  void Creation::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;
    CheckSavePrefab();

    for( World* worlds[]{ &sEditorWorld, &sGameWorld };
         World* world : worlds )
      world->mDebug3DDrawData->Clear();

    // Update the main window first so it becomes the parent window (maybe)
    TAC_CALL( CreationMainWindow::Update( errors ) );
    CreationSystemWindow::Update();
    TAC_CALL( CreationAssetView::Update( errors ) );
    TAC_CALL( CreationShaderGraphWindow::Update(  errors ) );
    TAC_CALL( CreationGameWindow::Update( errors ) );
    TAC_CALL( CreationPropertyWindow::Update( errors ) );
    TAC_CALL( CreationProfileWindow::Update( errors ) );

    if( sIsGameRunning )
      sGameWorld.Step( TAC_DT );
    else
    {
      sGameWorld.ComputeTransformsRecursively();
      sEditorWorld.ComputeTransformsRecursively();
    }

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

  auto Creation::InstantiateAsCopy( Entity* prefabEntity ) -> Entity*
  {
    const RelativeSpace relativeSpace{ GetEditorCameraVisibleRelativeSpace( GetCamera() ) };
    return InstantiateAsCopyAux( prefabEntity, relativeSpace );
  }

  auto Creation::CreateEntity() -> Entity*
  {
    World* world{ GetWorld() };
    Camera* camera{ GetCamera() };
    EntityUUIDCounter* entityUUIDCounter{GetEntityUUIDCounter()};
    Entity* entity{ world->SpawnEntity( entityUUIDCounter->AllocateNewUUID() ) };
    entity->mName = CreationGetNewEntityName( world );
    entity->mRelativeSpace = GetEditorCameraVisibleRelativeSpace( camera );
    SelectedEntities::Select( entity );
    return entity;
  }

  auto Creation::GetEditorCamera() -> Camera* { return &sEditorCamera; }

  auto Creation::GetCamera() -> Camera*
  {
    if( !sIsGameRunning )
      return &sEditorCamera;

    struct : public CameraVisitor
    {
      void operator()( CameraComponent* camera ) override { mCamera = camera; }
      CameraComponent* mCamera{};
    } cameraVisitor;
    Graphics::From( &sGameWorld )->VisitCameras( &cameraVisitor );
    return cameraVisitor.mCamera ? &cameraVisitor.mCamera->mCamera : nullptr;
  }

  auto Creation::GetWorld() -> World*   { return sIsGameRunning ? &sGameWorld : &sEditorWorld; }

  auto Creation::GetEntityUUIDCounter() -> EntityUUIDCounter*
  {
   return sIsGameRunning ? &sGameEntityUUIDCounter : &sEditorEntityUUIDCounter;
  }

  bool Creation::IsGameRunning()        { return sIsGameRunning; }

  bool Creation::CanShowWindow( StringView s )
  {
    return sShowOnlyWindowName.empty() || sShowOnlyWindowName == s;
  }
  void Creation::SetShowOnlyWindowName( StringView s ) { sShowOnlyWindowName = s; }

} // namespace Tac

