#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/graphics/tac_color_util.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/string/tac_string_util.h"
//#include "src/common/graphics/tac_font.h"
#include "src/common/graphics/tac_renderer.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/math/tac_math.h"
#include "src/common/meta/tac_meta_composite.h"
#include "src/common/meta/tac_meta_fn.h"
#include "src/common/meta/tac_meta_fn_sig.h"
#include "src/common/meta/tac_meta_var.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/core/tac_algorithm.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/system/tac_os.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/string/tac_string_util.h"
#include "src/level_editor/tac_level_editor.h"
#include "src/level_editor/tac_level_editor_asset_view.h"
#include "src/level_editor/tac_level_editor_game_window.h"
#include "src/level_editor/tac_level_editor_main_window.h"
#include "src/level_editor/tac_level_editor_prefab.h"
#include "src/level_editor/tac_level_editor_profile_window.h"
#include "src/level_editor/tac_level_editor_property_window.h"
#include "src/level_editor/tac_level_editor_system_window.h"
#include "src/shell/tac_desktop_app.h"
#include "src/space/model/tac_model.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/space/presentation/tac_shadow_presentation.h"
#include "src/space/presentation/tac_skybox_presentation.h"
#include "src/space/presentation/tac_voxel_gi_presentation.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_ghost.h"
#include "src/space/tac_space.h"
#include "src/space/tac_world.h"
#include "src/space/terrain/tac_terrain.h"

//#include "tac-core/test.h"

import std;
//#include <iostream>
//#include <functional>
//#include <algorithm>
//#include <array>

namespace Tac
{
  Creation gCreation;

  static struct CreatedWindowData
  {
    String              mName;
    int                 mX, mY, mW, mH;
    const void*         mNativeWindowHandle;
  } sCreatedWindowData[ kDesktopWindowCapacity ]{};

  static void   CreationInitCallback( Errors& errors )   { gCreation.Init( errors ); }
  static void   CreationUninitCallback( Errors& errors ) { gCreation.Uninit( errors ); }
  static void   CreationUpdateCallback( Errors& errors ) { gCreation.Update( errors ); }

  static String CreationGetNewEntityName()
  {
    World* world = gCreation.mWorld;
    String desiredEntityName = "Entity";
    int parenNumber = 1;
    for( ;; )
    {
      Entity* entity = world->FindEntity( desiredEntityName );
      if( !entity )
        break;
      desiredEntityName = "Entity (" + ToString( parenNumber ) + ")";
      parenNumber++;
    }
    return desiredEntityName;
  }

  static void   CheckSavePrefab()
  {
    World* world = gCreation.mWorld;
    const bool triggered =
      Keyboard::KeyboardIsKeyJustDown( Keyboard::Key::S ) &&
      Keyboard::KeyboardIsKeyDown( Keyboard::Key::Modifier );
    if( !triggered )
      return;

    Errors saveErrors;
    PrefabSave( world, saveErrors );

    CreationGameWindow* window = CreationGameWindow::Instance;
    if( window )
    {
      if( saveErrors )
      {
        const TimestampDifference errorDurationSecs = 60.0f;
        window->SetStatusMessage( saveErrors.ToString(), errorDurationSecs );
      }
      else
      {
        const TimestampDifference successDurationSecs = 5.0f;
        window->SetStatusMessage( "Saved prefabs!", successDurationSecs );
      }
    }
  }


  static void   AddCreatedWindowData( DesktopWindowHandle desktopWindowHandle,
                                    StringView name,
                                    int x, int y, int w, int h )
  {
    CreatedWindowData* createdWindowData = &sCreatedWindowData[ ( int )desktopWindowHandle ];
    createdWindowData->mName = name;
    createdWindowData->mX = x;
    createdWindowData->mY = y;
    createdWindowData->mW = w;
    createdWindowData->mH = h;
  }

  static void   UpdateCreatedWindowData()
  {
    for( int i = 0; i < kDesktopWindowCapacity; ++i )
    {
      const DesktopWindowState* desktopWindowState = GetDesktopWindowState( { i } );
      CreatedWindowData* createdWindowData = &sCreatedWindowData[ i ];
      if( createdWindowData->mNativeWindowHandle != desktopWindowState->mNativeWindowHandle )
      {
        createdWindowData->mNativeWindowHandle = desktopWindowState->mNativeWindowHandle;
        Json* json = gCreation.FindWindowJson( createdWindowData->mName );
        SettingsSetBool( "is_open",
                         (bool)desktopWindowState->mNativeWindowHandle,
                         json );
      }

      if( !desktopWindowState->mNativeWindowHandle )
        continue;

      const bool same =
        desktopWindowState->mX == createdWindowData->mX &&
        desktopWindowState->mY == createdWindowData->mY &&
        desktopWindowState->mWidth == createdWindowData->mW &&
        desktopWindowState->mHeight == createdWindowData->mH;
      if( same )
        continue;
      Json* json = gCreation.FindWindowJson( createdWindowData->mName );
      SettingsSetNumber( "x", createdWindowData->mX = desktopWindowState->mX, json );
      SettingsSetNumber( "y", createdWindowData->mY = desktopWindowState->mY, json );
      SettingsSetNumber( "w", createdWindowData->mW = desktopWindowState->mWidth, json );
      SettingsSetNumber( "h", createdWindowData->mH = desktopWindowState->mHeight, json );
    }
  }

  static bool   DoesAnyWindowExist()
  {
    for( int i = 0; i < kDesktopWindowCapacity; ++i )
      if( GetDesktopWindowState( { i } )->mNativeWindowHandle )
        return true;
    return false;
  }

  static bool   AllWindowsClosed()
  {
    static bool existed;
    const bool exists = DoesAnyWindowExist();
    existed |= exists;
    return !exists && existed;
  }


  //===-------------- SelectedEntities -------------===//

  void                SelectedEntities::AddToSelection( Entity* e ) { mSelectedEntities.push_back( e ); }

  void                SelectedEntities::DeleteEntities()
  {
    Vector< Entity* > topLevelEntitiesToDelete;

    for( Entity* entity : mSelectedEntities )
    {
      bool isTopLevel = true;
      for( Entity* parent = entity->mParent; parent; parent = parent->mParent )
      {
        if( Contains( mSelectedEntities, parent ) )
        {
          isTopLevel = false;
          break;
        }
      }
      if( isTopLevel )
        topLevelEntitiesToDelete.push_back( entity );
    }
    for( Entity* entity : topLevelEntitiesToDelete )
    {
      PrefabRemoveEntityRecursively( entity );
      gCreation.mWorld->KillEntity( entity );

    }
    mSelectedEntities.clear();
  }

  bool                SelectedEntities::IsSelected( Entity* e )
  {
    for( Entity* s : mSelectedEntities )
      if( s == e )
        return true;
    return false;
  }

  int                 SelectedEntities::size() const { return mSelectedEntities.size(); }

  Entity**            SelectedEntities::begin() { return mSelectedEntities.begin(); }

  Entity**            SelectedEntities::end() { return mSelectedEntities.end(); }

  void                SelectedEntities::Select( Entity* e ) { mSelectedEntities = { e }; }

  bool                SelectedEntities::empty() const
  {
    return mSelectedEntities.empty();
  }

  v3                  SelectedEntities::GetGizmoOrigin() const
  {
    TAC_ASSERT( !empty() );
    // do i really want average? or like center of bounding circle?
    v3 runningPosSum = {};
    int selectionCount = 0;
    for( Entity* entity : mSelectedEntities )
    {
      runningPosSum +=
        ( entity->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
      entity->mRelativeSpace.mPosition;
      selectionCount++;
    }
    v3 averagePos = runningPosSum / ( float )selectionCount;
    v3 result = averagePos;
    //if( mSelectedHitOffsetExists )
    //  result += mSelectedHitOffset;
    return result;
  }

  void                SelectedEntities::clear()
  {
    mSelectedEntities.clear();
    //mSelectedHitOffsetExists = false;
  }

  void                SelectedEntities::DeleteEntitiesCheck()
  {
    CreationGameWindow* gameWindow = CreationGameWindow::Instance;

    if( !gameWindow || !gameWindow->mDesktopWindowHandle.IsValid() )
      return;

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( gameWindow->mDesktopWindowHandle );

    if( !desktopWindowState->mNativeWindowHandle )
      return;

    if( !IsWindowHovered( gameWindow->mDesktopWindowHandle ) )
      return;

    if( !Keyboard::KeyboardIsKeyJustDown( Keyboard::Key::Delete ) )
      return;
    DeleteEntities();
  }

  //===-------------- ExecutableStartupInfo -------------===//

  ExecutableStartupInfo                ExecutableStartupInfo::Init()
  {
    const ProjectFns level_editorProjectFns
    {
      .mProjectInit = CreationInitCallback,
      .mProjectUpdate = CreationUpdateCallback,
      .mProjectUninit = CreationUninitCallback,
    };

    return ExecutableStartupInfo
    {
      .mAppName = "Creation",
      .mProjectFns = level_editorProjectFns,
    };
  }

  //===-------------- Creation -------------===//

  void                Creation::Init( Errors& errors )
  {
    MetaVarUnitTest();
    MetaFnSigUnitTest();
    MetaFnUnitTest();
    MetaCompositeUnitTest();

    mWorld = TAC_NEW World;
    mEditorCamera = TAC_NEW Camera{ .mPos = { 0, 1, 5 },
                                    .mForwards = { 0, 0, -1 },
                                    .mRight = { 1, 0, 0 },
                                    .mUp = { 0, 1, 0 } };

    SkyboxPresentationInit( errors );
    TAC_HANDLE_ERROR( errors );

    GamePresentationInit( errors );
    TAC_HANDLE_ERROR( errors );

    ShadowPresentationInit( errors );
    TAC_HANDLE_ERROR( errors );

    VoxelGIPresentationInit( errors );
    TAC_HANDLE_ERROR( errors );


    Json* windows;
    GetWindowsJson( &windows, errors );
    TAC_HANDLE_ERROR( errors );

    CreateInitialWindows( errors );
    TAC_HANDLE_ERROR( errors );

    PrefabLoad( &mEntityUUIDCounter, mWorld, mEditorCamera, errors );
    TAC_HANDLE_ERROR( errors );
  }

  void                Creation::Uninit( Errors& )
  {
    SkyboxPresentationUninit();
    GamePresentationUninit();
    VoxelGIPresentationUninit();
    ShadowPresentationUninit();
    TAC_DELETE CreationMainWindow::Instance;
    TAC_DELETE CreationGameWindow::Instance;
    TAC_DELETE CreationPropertyWindow::Instance;
    TAC_DELETE CreationSystemWindow::Instance;
    TAC_DELETE CreationProfileWindow::Instance;
  }

  void                Creation::CreatePropertyWindow( Errors& errors )
  {
    if( CreationPropertyWindow::Instance )
    {
      TAC_DELETE CreationPropertyWindow::Instance;
      return;
    }


    TAC_NEW CreationPropertyWindow;
    CreationPropertyWindow::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }

  void                Creation::CreateGameWindow( Errors& errors )
  {
    if( CreationGameWindow::Instance )
    {
      TAC_DELETE CreationGameWindow::Instance;
      return;
    }

    TAC_NEW CreationGameWindow;
    CreationGameWindow::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }

  void                Creation::CreateMainWindow( Errors& errors )
  {
    if( CreationMainWindow::Instance )
    {
      TAC_DELETE CreationMainWindow::Instance;
      return;
    }

    TAC_NEW CreationMainWindow;
    CreationMainWindow::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );
  }

  void                Creation::CreateSystemWindow( Errors& errors )
  {
    if( CreationSystemWindow::Instance )
    {
      TAC_DELETE CreationSystemWindow::Instance;
      return;
    }

    TAC_NEW CreationSystemWindow;
    CreationSystemWindow::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );

  }

  void                Creation::CreateProfileWindow( Errors& errors )
  {
    if( CreationProfileWindow::Instance )
    {
      TAC_DELETE CreationProfileWindow::Instance;
      return;
    }

    TAC_NEW CreationProfileWindow;
    CreationProfileWindow::Instance->Init( errors );
    TAC_HANDLE_ERROR( errors );

  }

  DesktopWindowHandle Creation::CreateWindow( StringView name )
  {
    int x, y, w, h;
    GetWindowsJsonData( name, &x, &y, &w, &h );
    DesktopWindowHandle desktopWindowHandle = DesktopAppCreateWindow( name, x, y, w, h );
    AddCreatedWindowData( desktopWindowHandle, name, x, y, w, h );

    DesktopAppMoveControls( desktopWindowHandle );
    DesktopAppResizeControls( desktopWindowHandle );
    return desktopWindowHandle;
  }

  void                Creation::GetWindowsJsonData( StringView windowName, int* x, int* y, int* w, int* h )
  {
    Json* windowJson = FindWindowJson( windowName );
    if( !windowJson )
    {
      Json* windows;
      Errors errors;
      GetWindowsJson( &windows, errors );
      windowJson = windows->AddChild();
      SettingsSetString( "Name", windowName, windowJson );
    }
    *w = ( int )SettingsGetNumber( "w", 400, windowJson );
    *h = ( int )SettingsGetNumber( "h", 300, windowJson );
    *x = ( int )SettingsGetNumber( "x", 200, windowJson );
    *y = ( int )SettingsGetNumber( "y", 200, windowJson );
  }

  void                Creation::GetWindowsJson( Json** outJson, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Json* windows = SettingsGetJson( "Windows" );
    *outJson = windows;
  }

  bool                Creation::ShouldCreateWindowNamed( StringView name )
  {
    mOnlyCreateWindowNamed = SettingsGetString( "onlyCreateWindowNamed", "" );
    if( mOnlyCreateWindowNamed.size() && name != mOnlyCreateWindowNamed )
      return false;
    Json* windowJson = FindWindowJson( name );
    if( !windowJson )
      return false;

    Errors errors;
    const bool create = SettingsGetBool( "is_open", false, windowJson );
    if( errors )
      return false;
    return create;
  }

  void                Creation::CreateInitialWindow( const char* name,
                                                     void ( Creation:: * fn )( Errors& ),
                                                     Errors& errors )
  {
    if( ShouldCreateWindowNamed( name ) )
      ( this->*fn )( errors );
  }

  void                Creation::CreateInitialWindows( Errors& errors )
  {
    Creation::CreateMainWindow( errors );
    CreateInitialWindow( gPropertyWindowName, &Creation::CreatePropertyWindow, errors );
    CreateInitialWindow( gGameWindowName, &Creation::CreateGameWindow, errors );
    CreateInitialWindow( gSystemWindowName, &Creation::CreateSystemWindow, errors );
    CreateInitialWindow( gProfileWindowName, &Creation::CreateProfileWindow, errors );
    TAC_HANDLE_ERROR( errors );
  }


  Json*               Creation::FindWindowJson( StringView windowName )
  {
    Json* windows;
    Errors errors;
    GetWindowsJson( &windows, errors );
    if( errors )
      return nullptr;
    return SettingsGetChildByKeyValuePair( "Name", Json( windowName ), windows );
  }


  void                Creation::Update( Errors& errors )
  {
    //debugCamera = mEditorCamera;

    TAC_PROFILE_BLOCK;


    CheckSavePrefab();
    UpdateCreatedWindowData();

    if( AllWindowsClosed() )
      OS::OSAppStopRunning();

    if( CreationMainWindow::Instance )
    {
      CreationMainWindow::Instance->Update( errors );
      if( CreationMainWindow::Instance->mCloseRequested )
        TAC_DELETE CreationMainWindow::Instance;
      TAC_HANDLE_ERROR( errors );
    }

    if( CreationGameWindow::Instance )
    {
      CreationGameWindow::Instance->Update( errors );
      if( CreationGameWindow::Instance->mCloseRequested )
        TAC_DELETE CreationGameWindow::Instance;
      TAC_HANDLE_ERROR( errors );
    }

    if( CreationPropertyWindow::Instance )
    {
      CreationPropertyWindow::Instance->Update( errors );
      if( CreationPropertyWindow::Instance->mCloseRequested )
        TAC_DELETE CreationPropertyWindow::Instance;
      TAC_HANDLE_ERROR( errors );
    }

    if( CreationSystemWindow::Instance )
    {
      CreationSystemWindow::Instance->Update( errors );
      if( CreationSystemWindow::Instance->mCloseRequested )
        TAC_DELETE CreationSystemWindow::Instance;
      TAC_HANDLE_ERROR( errors );
    }

    if( CreationProfileWindow::Instance )
    {
      CreationProfileWindow::Instance->Update( errors );
      if( CreationProfileWindow::Instance->mCloseRequested )
        TAC_DELETE CreationProfileWindow::Instance;
      TAC_HANDLE_ERROR( errors );
    }

    if( mUpdateAssetView )
      CreationUpdateAssetView();

    mWorld->Step( TAC_DELTA_FRAME_SECONDS );


    mSelectedEntities.DeleteEntitiesCheck();
  }


  RelativeSpace       Creation::GetEditorCameraVisibleRelativeSpace()
  {
    const v3 pos = mEditorCamera->mPos + mEditorCamera->mForwards * 5.0f;
    RelativeSpace relativeSpace;
    relativeSpace.mPosition = pos;
    return relativeSpace;
  }

  Entity*             Creation::InstantiateAsCopy( Entity* prefabEntity, const RelativeSpace& relativeSpace )
  {
    Entity* copyEntity = CreateEntity();
    copyEntity->mRelativeSpace = relativeSpace;
    copyEntity->mInheritParentScale = prefabEntity->mInheritParentScale;
    copyEntity->mName = prefabEntity->mName;

    for( Component* prefabComponent : prefabEntity->mComponents )
    {
      const ComponentRegistryEntry* entry = prefabComponent->GetEntry();
      Component* copyComponent = copyEntity->AddNewComponent( prefabComponent->GetEntry() );
      Json dOnT_mInD_iF_i_dO;
      entry->mSaveFn( dOnT_mInD_iF_i_dO, prefabComponent );
      entry->mLoadFn( dOnT_mInD_iF_i_dO, copyComponent );
    }

    for( Entity* prefabChildEntity : prefabEntity->mChildren )
    {
      Entity* copyChildEntity = InstantiateAsCopy( prefabChildEntity, prefabChildEntity->mRelativeSpace );
      //Entity* copyChildEntity = CreateEntity();
      copyEntity->AddChild( copyChildEntity );
    }
    return copyEntity;
  }

  Entity*             Creation::CreateEntity()
  {
    World* world = mWorld;
    Entity* entity = world->SpawnEntity( mEntityUUIDCounter.AllocateNewUUID() );
    entity->mName = CreationGetNewEntityName();
    entity->mRelativeSpace = GetEditorCameraVisibleRelativeSpace();
    mSelectedEntities.Select( entity );
    return entity;
  }



  /*
  void                ModifyPathRelative( Filesystem::Path& savePath )
  {
    const Filesystem::Path workingDir = ShellGetInitialWorkingDir();

    if( savePath.starts_with(workingDir ))
    {
      savePath = savePath.substr( StrLen( workingDir ) );
      savePath = Filesystem::StripLeadingSlashes( savePath );
    }
  }
  */

}

