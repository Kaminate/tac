#include "src/level_editor/tac_level_editor_window_manager.h" // self-inc

#include "src/common/core/tac_error_handling.h"
#include "src/common/core/tac_preprocessor.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/profile/tac_profile.h"
#include "src/common/system/tac_os.h"
#include "src/level_editor/tac_level_editor_asset_view.h"
#include "src/level_editor/tac_level_editor_game_window.h"
#include "src/level_editor/tac_level_editor_main_window.h"
#include "src/level_editor/tac_level_editor_prefab.h"
#include "src/level_editor/tac_level_editor_profile_window.h"
#include "src/level_editor/tac_level_editor_property_window.h"
#include "src/level_editor/tac_level_editor_system_window.h"
#include "src/shell/tac_desktop_app.h"

import std;

namespace Tac
{

  struct CreatedWindowData
  {
    String              mName;
    int                 mX;
    int                 mY;
    int                 mW;
    int                 mH;
    const void*         mNativeWindowHandle = nullptr;
  };
  
  static CreatedWindowData sCreatedWindowData[ kDesktopWindowCapacity ]{};



  void   LevelEditorWindowManager::UpdateCreatedWindowData()
  {
    for( int i = 0; i < kDesktopWindowCapacity; ++i )
    {
      const DesktopWindowState* desktopWindowState = GetDesktopWindowState( { i } );
      CreatedWindowData* createdWindowData = &sCreatedWindowData[ i ];
      if( createdWindowData->mNativeWindowHandle != desktopWindowState->mNativeWindowHandle )
      {
        createdWindowData->mNativeWindowHandle = desktopWindowState->mNativeWindowHandle;
        Json* json = FindWindowJson( createdWindowData->mName );
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

      Json* json = FindWindowJson( createdWindowData->mName );
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

  bool   LevelEditorWindowManager::AllWindowsClosed()
  {
    static bool existed;
    const bool exists = DoesAnyWindowExist();
    existed |= exists;
    return !exists && existed;
  }




  void                LevelEditorWindowManager::Uninit( Errors& )
  {

    TAC_DELETE CreationMainWindow::Instance;
    TAC_DELETE CreationGameWindow::Instance;
    TAC_DELETE CreationPropertyWindow::Instance;
    TAC_DELETE CreationSystemWindow::Instance;
    TAC_DELETE CreationProfileWindow::Instance;
  }

  void                LevelEditorWindowManager::CreatePropertyWindow( Errors& errors )
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

  void                LevelEditorWindowManager::CreateGameWindow( Errors& errors )
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

  void                LevelEditorWindowManager::CreateMainWindow( Errors& errors )
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

  void                LevelEditorWindowManager::CreateSystemWindow( Errors& errors )
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

  void                LevelEditorWindowManager::CreateProfileWindow( Errors& errors )
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

  DesktopWindowHandle LevelEditorWindowManager::CreateDesktopWindow( StringView name )
  {
    const DesktopAppCreateWindowParams createParams = GetWindowsJsonData( name );
    const DesktopWindowHandle desktopWindowHandle = DesktopAppCreateWindow( createParams );
    sCreatedWindowData[ desktopWindowHandle.GetIndex() ] =
      CreatedWindowData
    {
      .mName = name,
      .mX = createParams.mX,
      .mY = createParams.mY,
      .mW = createParams.mWidth,
      .mH = createParams.mHeight,
      .mNativeWindowHandle = nullptr,
    };

    DesktopAppMoveControls( desktopWindowHandle );
    DesktopAppResizeControls( desktopWindowHandle );
    return desktopWindowHandle;
  }

  DesktopAppCreateWindowParams LevelEditorWindowManager::GetWindowsJsonData( StringView name )
  {
    Json* windowJson = FindWindowJson( name );
    if( !windowJson )
    {
      Json* windows = GetWindowsJson();
      windowJson = windows->AddChild();
      SettingsSetString( "Name", name, windowJson );
    }

    const DesktopAppCreateWindowParams createParams
    {
      .mName = name,
      .mX = ( int )SettingsGetNumber( "x", 200, windowJson ),
      .mY = ( int )SettingsGetNumber( "y", 200, windowJson ),
      .mWidth = ( int )SettingsGetNumber( "w", 400, windowJson ),
      .mHeight = ( int )SettingsGetNumber( "h", 300, windowJson ),
    };
    return createParams;
  }

  Json*               LevelEditorWindowManager::GetWindowsJson()
  {
    return SettingsGetJson( "Windows" );
  }

  bool                LevelEditorWindowManager::ShouldCreateWindowNamed( StringView name )
  {
    const String mOnlyCreateWindowNamed = SettingsGetString( "onlyCreateWindowNamed", "" );
    if( !mOnlyCreateWindowNamed.empty() && name != mOnlyCreateWindowNamed )
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


  void                LevelEditorWindowManager::CreateInitialWindows( Errors& errors )
  {
    CreateMainWindow( errors );

    using Fn = void(LevelEditorWindowManager::*)( Errors&);

    const struct
    {
      const char* mName;
      const Fn mFn;
    } params [] =
    {
      { gPropertyWindowName, &LevelEditorWindowManager::CreatePropertyWindow},
      { gGameWindowName, &LevelEditorWindowManager::CreateGameWindow},
      { gSystemWindowName, &LevelEditorWindowManager::CreateSystemWindow},
      { gProfileWindowName, &LevelEditorWindowManager::CreateProfileWindow},
    };

    for( auto [name, fn] : params )
    {
      if( ShouldCreateWindowNamed( name ) )
      {
        ( this->*fn )( errors );
        TAC_HANDLE_ERROR( errors );
      }
    }
  }


  Json*               LevelEditorWindowManager::FindWindowJson( StringView windowName )
  {
    Json* windows = GetWindowsJson();
    if( !windows )
      return nullptr;

    return SettingsGetChildByKeyValuePair( "Name", Json( windowName ), windows );
  }

  static void UpdateWindows(Errors& errors)
  {
    TAC_PROFILE_BLOCK;

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
  }

  void                LevelEditorWindowManager::Update( Errors& errors )
  {

    UpdateCreatedWindowData();

    UpdateWindows(errors);

  }




} // namespace Tac

