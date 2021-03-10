#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacColorUtil.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacUtility.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacTime.h"
#include "src/common/tacOS.h"
#include "src/common/tacShellTimer.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationPrefab.h"
#include "src/creation/tacCreationGameWindow.h"
#include "src/creation/tacCreationPropertyWindow.h"
#include "src/creation/tacCreationMainWindow.h"
#include "src/creation/tacCreationSystemWindow.h"
#include "src/creation/tacCreationProfileWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/space/tacGhost.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"
#include "src/space/model/tacModel.h"
#include "src/space/terrain/tacTerrain.h"
#include "src/space/tacSpace.h"
#include <iostream>
#include <functional>
#include <algorithm>

namespace Tac
{
  Creation gCreation;
  static void CreationInitCallback( Errors& errors ) { gCreation.Init( errors ); }
  static void CreationUninitCallback( Errors& errors ) { gCreation.Uninit( errors ); }
  static void CreationUpdateCallback( Errors& errors ) { gCreation.Update( errors ); }
  static struct CreatedWindowData
  {
    String              mName;
    int                 mX, mY, mW, mH;
    const void*         mNativeWindowHandle;
  } sCreatedWindowData[ kDesktopWindowCapacity ];
  static void AddCreatedWindowData( DesktopWindowHandle desktopWindowHandle,
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

  static void UpdateCreatedWindowData()
  {
    for( int i = 0; i < kDesktopWindowCapacity; ++i )
    {
      DesktopWindowHandle desktopWindowHandle = { i };
      DesktopWindowState* desktopWindowState = GetDesktopWindowState( desktopWindowHandle );
      CreatedWindowData* createdWindowData = &sCreatedWindowData[ i ];


      if( createdWindowData->mNativeWindowHandle != desktopWindowState->mNativeWindowHandle )
      {
        Json* json = gCreation.FindWindowJson( createdWindowData->mName );
        SettingsSetBool( "is_open",
                         createdWindowData->mNativeWindowHandle = desktopWindowState->mNativeWindowHandle  ,
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

  static bool AllWindowsClosed()
  {
    static int maxWindowCount;
    int curWindowCount = 0;
    for( int i = 0; i < kDesktopWindowCapacity; ++i )
    {
      DesktopWindowState* desktopWindowState = GetDesktopWindowState( { i } );
      curWindowCount += desktopWindowState->mNativeWindowHandle ? 1 : 0;
    }
    maxWindowCount = Max( maxWindowCount, curWindowCount );
    return maxWindowCount && !curWindowCount;
  }

  void ExecutableStartupInfo::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mAppName = "Creation";
    mProjectInit = CreationInitCallback;
    mProjectUpdate = CreationUpdateCallback;
    mProjectUninit = CreationUninitCallback;
  }

  void                Creation::Uninit( Errors& errors )
  {
    delete CreationMainWindow::Instance;
    delete CreationGameWindow::Instance;
    delete CreationPropertyWindow::Instance;
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
    DesktopWindowHandle desktopWindowHandle = DesktopAppCreateWindow( x, y, w, h );
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
      GetWindowsJson( &windows, Errors() );
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

  //void Creation::CreateInitialWindow( const char * name, void( Creation::* )( Errors & ), Errors & errors )
  //{
  //}

  void                Creation::CreateInitialWindows( Errors& errors )
  {
    CreateInitialWindow( gMainWindowName, &Creation::CreateMainWindow, errors );
    CreateInitialWindow( gPropertyWindowName, &Creation::CreatePropertyWindow, errors );
    CreateInitialWindow( gGameWindowName, &Creation::CreateGameWindow, errors );
    CreateInitialWindow( gSystemWindowName, &Creation::CreateSystemWindow, errors );
    CreateInitialWindow( gProfileWindowName, &Creation::CreateProfileWindow, errors );
    TAC_HANDLE_ERROR( errors );
  }

  void                Creation::Init( Errors& errors )
  {
    SpaceInit();
    mWorld = TAC_NEW World;
    mEditorCamera.mPos = { 0, 1, 5 };
    mEditorCamera.mForwards = { 0, 0, -1 };
    mEditorCamera.mRight = { 1, 0, 0 };
    mEditorCamera.mUp = { 0, 1, 0 };

    String dataPath;
    OS::GetApplicationDataPath( dataPath, errors );
    TAC_HANDLE_ERROR( errors );

    OS::CreateFolderIfNotExist( dataPath, errors );
    TAC_HANDLE_ERROR( errors );

    Json* windows;
    GetWindowsJson( &windows, errors );
    TAC_HANDLE_ERROR( errors );


    CreateInitialWindows( errors );
    TAC_HANDLE_ERROR( errors );

    PrefabLoad( mWorld, &mEditorCamera, errors );
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

    //for( Json* windowJson : windows->mArrayElements )
    //  if( windowJson->GetChild( "Name" ) == windowName )
    //    return windowJson;
    //return nullptr;
  }


  void                Creation::DeleteSelectedEntities()
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
      mWorld->KillEntity( entity );
    }
    mSelectedEntities.clear();
  }

  void                Creation::Update( Errors& errors )
  {

    /*TAC_PROFILE_BLOCK*/;


    UpdateCreatedWindowData();

    if( AllWindowsClosed() )
      OS::StopRunning();



    if( !CreationMainWindow::Instance &&
        !CreationGameWindow::Instance &&
        !CreationPropertyWindow::Instance &&
        !CreationSystemWindow::Instance &&
        !CreationProfileWindow::Instance )
    {
      CreateMainWindow( errors );
      //CreateGameWindow( errors );
      //CreatePropertyWindow( errors );
      //CreateSystemWindow( errors );
      TAC_HANDLE_ERROR( errors );
    }

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
      TAC_HANDLE_ERROR( errors );
    }

    if( CreationProfileWindow::Instance )
    {
      CreationProfileWindow::Instance->Update( errors );
      TAC_HANDLE_ERROR( errors );
    }

    mWorld->Step( TAC_DELTA_FRAME_SECONDS );


    CheckDeleteSelected();

    if( gKeyboardInput.IsKeyJustDown( Key::S ) &&
        gKeyboardInput.IsKeyDown( Key::Modifier ) )
    {
      PrefabSave( mWorld );
      if( CreationGameWindow::Instance )
      {
        CreationGameWindow::Instance->mStatusMessage = "Saved prefabs!";
        CreationGameWindow::Instance->mStatusMessageEndTime = ShellGetElapsedSeconds() + 5.0f;
      }
    }


  }

  Entity*             Creation::CreateEntity()
  {
    World* world = mWorld;
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

    Entity* entity = world->SpawnEntity( NullEntityUUID );
    entity->mName = desiredEntityName;
    mSelectedEntities = { entity };
    return entity;
  }

  bool                Creation::IsAnythingSelected()
  {
    return mSelectedEntities.size();
  }

  v3                  Creation::GetSelectionGizmoOrigin()
  {
    TAC_ASSERT( IsAnythingSelected() );
    // do i really want average? or like center of bounding circle?
    v3 runningPosSum = {};
    int selectionCount = 0;
    for( auto entity : mSelectedEntities )
    {
      runningPosSum +=
        ( entity->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
      entity->mRelativeSpace.mPosition;
      selectionCount++;
    }
    v3 averagePos = runningPosSum / ( float )selectionCount;
    v3 result = averagePos;
    if( mSelectedHitOffsetExists )
      result += mSelectedHitOffset;
    return result;
  }

  void                Creation::ClearSelection()
  {
    mSelectedEntities.clear();
    mSelectedHitOffsetExists = false;
  }

  void                Creation::CheckDeleteSelected()
  {
    CreationGameWindow* gameWindow = CreationGameWindow::Instance;

    if( !gameWindow || !gameWindow->mDesktopWindowHandle.IsValid() )
      return;

    DesktopWindowState* desktopWindowState = GetDesktopWindowState( gameWindow->mDesktopWindowHandle );

    if( !desktopWindowState->mNativeWindowHandle )
      return;

    if( !IsWindowHovered( gameWindow->mDesktopWindowHandle ) )
      return;

    if( !gKeyboardInput.IsKeyJustDown( Key::Delete ) )
      return;
    DeleteSelectedEntities();
  }


  void                ModifyPathRelative( String& savePath )
  {
    if( StartsWith( savePath, ShellGetInitialWorkingDir() ) )
    {
      savePath = savePath.substr( String( ShellGetInitialWorkingDir() ).size() );
      savePath = StripLeadingSlashes( savePath );
    }
  }

}

