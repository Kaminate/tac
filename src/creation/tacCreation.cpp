#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacColorUtil.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacTime.h"
#include "src/common/tacOS.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacTemporaryMemory.h"
#include "src/creation/tacCreation.h"
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
  static Creation gCreation;
  static void CreationInitCallback( Errors& errors ) { gCreation.Init( errors ); }
  static void CreationUpdateCallback( Errors& errors ) { gCreation.Update( errors ); }
  const char* prefabSettingsPath = "prefabs";
  const char* refFrameVecNames[] = {
    "mPos",
    "mForwards",
    "mRight",
    "mUp",
  };
  const char* axisNames[] = { "x", "y", "z" };

  void ExecutableStartupInfo::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mAppName = "Creation";
    mStudioName = "Sleeping Studio";
    mProjectInit = CreationInitCallback;
    mProjectUpdate = CreationUpdateCallback;
    //TAC_NEW Creation;
  }

  Creation* Creation::Instance = nullptr;

  Creation::Creation()
  {
    Instance = this;
  }

  Creation::~Creation()
  {
    delete CreationMainWindow::Instance;
    delete CreationGameWindow::Instance;
    delete CreationPropertyWindow::Instance;
  }

  void Creation::CreatePropertyWindow( Errors& errors )
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

  void Creation::CreateGameWindow( Errors& errors )
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

  void Creation::CreateMainWindow( Errors& errors )
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

  void Creation::CreateSystemWindow( Errors& errors )
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

  void Creation::CreateProfileWindow( Errors& errors )
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
    return DesktopWindowCreate( x, y, w, h );
  }

  void Creation::GetWindowsJsonData( String windowName, int* x, int* y, int* w, int* h )
  {
    Json* windowJson = FindWindowJson( windowName );
    if( !windowJson )
    {
      *x = 200;
      *y = 200;
      *w = 400;
      *h = 300;
      return;
    }

    Settings* settings = Settings::Instance;
    Errors errors;

    *w = ( int )settings->GetNumber( windowJson, { "w" }, 400, errors );
    *h = ( int )settings->GetNumber( windowJson, { "h" }, 300, errors );
    *x = ( int )settings->GetNumber( windowJson, { "x" }, 200, errors );
    *y = ( int )settings->GetNumber( windowJson, { "y" }, 200, errors );
    const bool centered = ( int )settings->GetBool( windowJson, { "centered" }, false, errors );
    TAC_HANDLE_ERROR( errors );

    if( centered )
    {
      CenterWindow( x, y, *w, *h );
    }
  }

  void Creation::GetWindowsJson( Json** outJson, Errors& errors )
  {
    Settings* settings = Settings::Instance;
    Vector< String > settingsPaths = { "Windows" };
    auto windowDefault = TAC_NEW Json;
    ( *windowDefault )[ "Name" ] = gMainWindowName;

    auto windowsDefault = TAC_NEW Json;
    windowsDefault->mType = JsonType::Array;
    windowsDefault->mElements.push_back( windowDefault );
    Json* windows = settings->GetArray( nullptr, { "Windows" }, windowsDefault, errors );
    TAC_HANDLE_ERROR( errors );

    *outJson = windows;
  }

  bool Creation::ShouldCreateWindowNamed( StringView name )
  {
    if( mOnlyCreateWindowNamed.size() && name != mOnlyCreateWindowNamed )
      return false;
    Json* windowJson = FindWindowJson( name );
    if( !windowJson )
      return false;

    Settings* settings = Settings::Instance;
    Errors errors;
    const bool create = settings->GetBool( windowJson, { "Create" }, false, errors );
    if( errors )
      return false;
    return create;
  }

  void Creation::SetSavedWindowData( Json* windowJson, Errors& errors )
  {
    Settings* settings = Settings::Instance;

    const StringView name = settings->GetString( windowJson, { "Name" }, gMainWindowName, errors );
    TAC_HANDLE_ERROR( errors );

    const int width = ( int )settings->GetNumber( windowJson, { "w" }, 400, errors );
    TAC_HANDLE_ERROR( errors );

    const int height = ( int )settings->GetNumber( windowJson, { "h" }, 300, errors );
    TAC_HANDLE_ERROR( errors );

    int x = ( int )settings->GetNumber( windowJson, { "x" }, 200, errors );
    TAC_HANDLE_ERROR( errors );

    int y = ( int )settings->GetNumber( windowJson, { "y" }, 200, errors );
    TAC_HANDLE_ERROR( errors );

    const bool centered = ( int )settings->GetBool( windowJson, { "centered" }, false, errors );
    TAC_HANDLE_ERROR( errors );

    if( centered )
    {
      CenterWindow( &x, &y, width, height );
    }

    //WindowParams params;
    //params.mName = name;
    //params.mWidth = width;
    //params.mHeight = height;
    //params.mX = x;
    //params.mY = y;
    //DesktopWindowManager::Instance->SetWindowParams( params );
  }

  void Creation::SetSavedWindowsData( Errors& errors )
  {

    Json* windows;
    GetWindowsJson( &windows, errors );
    TAC_HANDLE_ERROR( errors );


    for( Json* windowJson : windows->mElements )
    {
      SetSavedWindowData( windowJson, errors );
      TAC_HANDLE_ERROR( errors );
    }
  }

  void Creation::Init( Errors& errors )
  {
    SpaceInit();
    mWorld = TAC_NEW World;
    mEditorCamera.mPos = { 0, 1, 5 };
    mEditorCamera.mForwards = { 0, 0, -1 };
    mEditorCamera.mRight = { 1, 0, 0 };
    mEditorCamera.mUp = { 0, 1, 0 };

    //ViewIdMainWindow = Render::CreateViewId();
    //ViewIdGameWindow = Render::CreateViewId();
    //ViewIdPropertyWindow = Render::CreateViewId();
    //ViewIdSystemWindow = Render::CreateViewId();
    //ViewIdProfileWindow = Render::CreateViewId();

    String dataPath;
    OS::GetApplicationDataPath( dataPath, errors );
    TAC_HANDLE_ERROR( errors );

    OS::CreateFolderIfNotExist( dataPath, errors );
    TAC_HANDLE_ERROR( errors );

    Settings* settings = Settings::Instance;

    SetSavedWindowsData( errors );
    TAC_HANDLE_ERROR( errors );

    Json* windows;
    GetWindowsJson( &windows, errors );
    TAC_HANDLE_ERROR( errors );

    mOnlyCreateWindowNamed = settings->GetString( nullptr,
                                                  { "onlyCreateWindowNamed" },
                                                  "",
                                                  errors );
    TAC_HANDLE_ERROR( errors );

    // The first window spawned becomes the parent window
    if( ShouldCreateWindowNamed( gMainWindowName ) )
      CreateMainWindow( errors );
    TAC_HANDLE_ERROR( errors );

    if( ShouldCreateWindowNamed( gPropertyWindowName ) )
      CreatePropertyWindow( errors );
    TAC_HANDLE_ERROR( errors );

    if( ShouldCreateWindowNamed( gGameWindowName ) )
      CreateGameWindow( errors );
    TAC_HANDLE_ERROR( errors );

    if( ShouldCreateWindowNamed( gSystemWindowName ) )
      CreateSystemWindow( errors );
    TAC_HANDLE_ERROR( errors );

    if( ShouldCreateWindowNamed( gProfileWindowName ) )
      CreateProfileWindow( errors );
    TAC_HANDLE_ERROR( errors );


    LoadPrefabs( errors );
    TAC_HANDLE_ERROR( errors );
  }

  Json* Creation::FindWindowJson( StringView windowName )
  {
    Json* windows;
    Errors errors;
    GetWindowsJson( &windows, errors );
    if( errors )
      return nullptr;
    for( Json* windowJson : windows->mElements )
      if( windowJson->GetChild( "Name" ) == windowName )
        return windowJson;
    return nullptr;
  }

  void Creation::RemoveEntityFromPrefabRecursively( Entity* entity )
  {
    int prefabCount = mPrefabs.size();
    for( int iPrefab = 0; iPrefab < prefabCount; ++iPrefab )
    {
      Prefab* prefab = mPrefabs[ iPrefab ];
      bool removedEntityFromPrefab = false;
      int prefabEntityCount = prefab->mEntities.size();
      for( int iPrefabEntity = 0; iPrefabEntity < prefabEntityCount; ++iPrefabEntity )
      {
        if( prefab->mEntities[ iPrefabEntity ] == entity )
        {
          prefab->mEntities[ iPrefabEntity ] = prefab->mEntities[ prefabEntityCount - 1 ];
          prefab->mEntities.pop_back();
          if( prefab->mEntities.empty() )
          {
            mPrefabs[ iPrefab ] = mPrefabs[ prefabCount - 1 ];
            mPrefabs.pop_back();
          }

          removedEntityFromPrefab = true;
          break;
        }
      }

      if( removedEntityFromPrefab )
        break;
    }
    for( Entity* child : entity->mChildren )
      RemoveEntityFromPrefabRecursively( child );
  }

  void Creation::DeleteSelectedEntities()
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
      RemoveEntityFromPrefabRecursively( entity );
      mWorld->KillEntity( entity );
    }
    mSelectedEntities.clear();
  }

  void Creation::Update( Errors& errors )
  {
    /*TAC_PROFILE_BLOCK*/;

    DesktopEventQueue::Instance.ApplyQueuedEvents(GetDesktopWindowState(0));

    if( !CreationMainWindow::Instance &&
        !CreationGameWindow::Instance &&
        !CreationPropertyWindow::Instance &&
        !CreationSystemWindow::Instance &&
        !CreationProfileWindow::Instance )
    {
      CreateMainWindow( errors );
      CreateGameWindow( errors );
      //CreateSystemWindow( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( CreationMainWindow::Instance )
    {
      CreationMainWindow::Instance->Update( errors );
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

    if( KeyboardInput::Instance->IsKeyJustDown( Key::S ) &&
        KeyboardInput::Instance->IsKeyDown( Key::Modifier ) )
    {
      SavePrefabs();
      if( CreationGameWindow::Instance )
      {
        CreationGameWindow::Instance->mStatusMessage = "Saved prefabs!";
        CreationGameWindow::Instance->mStatusMessageEndTime = Shell::Instance.mElapsedSeconds + 5.0f;
      }
    }


  }

  Entity* Creation::CreateEntity()
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

  bool Creation::IsAnythingSelected()
  {
    return mSelectedEntities.size();
  }

  v3 Creation::GetSelectionGizmoOrigin()
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

  void Creation::ClearSelection()
  {
    mSelectedEntities.clear();
    mSelectedHitOffsetExists = false;
  }

  void Creation::CheckDeleteSelected()
  {
    CreationGameWindow* gameWindow = CreationGameWindow::Instance;

    if( !gameWindow || !gameWindow->mDesktopWindowHandle.IsValid() )
      return;

    DesktopWindowState* desktopWindowState = GetDesktopWindowState(gameWindow->mDesktopWindowHandle);

if(!desktopWindowState->mNativeWindowHandle)
      return;
    //if( !desktopWindowState->mCursorUnobscured )
    //return;

    if( !KeyboardInput::Instance->IsKeyJustDown( Key::Delete ) )
      return;
    DeleteSelectedEntities();
  }

  void Creation::GetSavedPrefabs( Vector< String > & paths, Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    Json& prefabs = Settings::Instance->mJson[ prefabSettingsPath ];
    prefabs.mType = JsonType::Array;

    Vector< String > alreadySavedPrefabs;
    for( Json* child : prefabs.mElements )
      alreadySavedPrefabs.push_back( child->mString );
    paths = alreadySavedPrefabs;
  }

  void Creation::UpdateSavedPrefabs()
  {
    Json& prefabs = Settings::Instance->mJson[ prefabSettingsPath ];
    prefabs.mType = JsonType::Array;

    Errors errors;
    Vector< String > alreadySavedPrefabs;
    GetSavedPrefabs( alreadySavedPrefabs, errors );
    TAC_HANDLE_ERROR( errors );

    for( Prefab* prefab : mPrefabs )
    {
      if( prefab->mDocumentPath.empty() )
        continue;
      if( Contains( alreadySavedPrefabs, prefab->mDocumentPath ) )
        continue;
      prefabs.mElements.push_back( TAC_NEW Json( prefab->mDocumentPath ) );
    }

    Settings::Instance->Save( errors );
  }

  Prefab* Creation::FindPrefab( Entity* entity )
  {
    for( Prefab* prefab : mPrefabs )
    {
      if( Contains( prefab->mEntities, entity ) )
      {
        return prefab;
      }
    }
    return nullptr;
  }

  void Creation::SavePrefabs()
  {
    for( Entity* entity : mWorld->mEntities )
    {
      if( entity->mParent )
        continue;

      Prefab* prefab = FindPrefab( entity );
      if( !prefab )
      {
        prefab = TAC_NEW Prefab;
        prefab->mEntities = { entity };
        mPrefabs.push_back( prefab );
      }


      // Get document paths for prefabs missing them
      if( prefab->mDocumentPath.empty() )
      {
        String savePath;
        String suggestedName =
          //prefab->mEntity->mName
          entity->mName +
          ".prefab";
        Errors saveDialogErrors;
        OS::SaveDialog( savePath, suggestedName, saveDialogErrors );
        if( saveDialogErrors )
        {
          // todo: log it, user feedback
          std::cout << saveDialogErrors.ToString() << std::endl;
          continue;
        }

        ModifyPathRelative( savePath );

        prefab->mDocumentPath = savePath;
        UpdateSavedPrefabs();
      }

      //Entity* entity = prefab->mEntity;

      Json entityJson;
      entity->Save( entityJson );

      String prefabJsonString = entityJson.Stringify();
      Errors saveToFileErrors;
      void* bytes = prefabJsonString.data();
      int byteCount = prefabJsonString.size();
      OS::SaveToFile( prefab->mDocumentPath, bytes, byteCount, saveToFileErrors );
      if( saveToFileErrors )
      {
        // todo: log it, user feedback
        std::cout << saveToFileErrors.ToString() << std::endl;
        continue;
      }
    }
  }

  void Creation::ModifyPathRelative( String& savePath )
  {
    if( StartsWith( savePath, Shell::Instance.mInitialWorkingDir ) )
    {
      savePath = savePath.substr( Shell::Instance.mInitialWorkingDir.size() );
      savePath = StripLeadingSlashes( savePath );
    }
  }

  void Creation::LoadPrefabAtPath( String prefabPath, Errors& errors )
  {
    ModifyPathRelative( prefabPath );
    auto memory = TemporaryMemoryFromFile( prefabPath, errors );
    TAC_HANDLE_ERROR( errors );

    Json prefabJson;
    prefabJson.Parse( memory.data(), memory.size(), errors );

    Entity* entity = mWorld->SpawnEntity( NullEntityUUID );
    entity->Load( prefabJson );

    auto prefab = TAC_NEW Prefab;
    prefab->mDocumentPath = prefabPath;
    prefab->mEntities = { entity };
    mPrefabs.push_back( prefab );

    LoadPrefabCameraPosition( prefab );
    UpdateSavedPrefabs();
  }

  void Creation::LoadPrefabs( Errors& errors )
  {
    Vector< String > prefabPaths;
    GetSavedPrefabs( prefabPaths, errors );
    for( auto& prefabPath : prefabPaths )
    {
      LoadPrefabAtPath( prefabPath, errors );
      TAC_HANDLE_ERROR( errors );
    }
  }

  void Creation::LoadPrefabCameraPosition( Prefab* prefab )
  {
    if( prefab->mDocumentPath.empty() )
      return;
    Settings* settings = Settings::Instance;
    Json* root = nullptr;
    v3* refFrameVecs[] = {
      &mEditorCamera.mPos,
      &mEditorCamera.mForwards,
      &mEditorCamera.mRight,
      &mEditorCamera.mUp,
    };
    for( int iRefFrameVec = 0; iRefFrameVec < 4; ++iRefFrameVec )
    {
      v3* refFrameVec = refFrameVecs[ iRefFrameVec ];
      const StringView refFrameVecName = refFrameVecNames[ iRefFrameVec ];
      for( int iAxis = 0; iAxis < 3; ++iAxis )
      {
        const StringView axisName = axisNames[ iAxis ];

        Vector< String > settingsPath = {
          "prefabCameraRefFrames",
          prefab->mDocumentPath,
          refFrameVecName,
          axisName };

        Errors ignored;
        JsonNumber defaultValue = refFrameVecs[ iRefFrameVec ]->operator[]( iAxis );
        JsonNumber axisValue = settings->GetNumber(
          root,
          settingsPath,
          defaultValue,
          ignored );

        refFrameVec->operator[]( iAxis ) = ( float )axisValue;
      }
    }
  }

  void Creation::SavePrefabCameraPosition( Prefab* prefab )
  {
    if( prefab->mDocumentPath.empty() )
      return;
    Json* root = nullptr;
    Settings* settings = Settings::Instance;

    v3 refFrameVecs[] = {
      mEditorCamera.mPos,
      mEditorCamera.mForwards,
      mEditorCamera.mRight,
      mEditorCamera.mUp,
    };
    for( int iRefFrameVec = 0; iRefFrameVec < 4; ++iRefFrameVec )
    {
      v3 refFrameVec = refFrameVecs[ iRefFrameVec ];
      String refFrameVecName = refFrameVecNames[ iRefFrameVec ];
      for( int iAxis = 0; iAxis < 3; ++iAxis )
      {
        const StringView axisName = axisNames[ iAxis ];

        Vector< String > settingsPath =
        {
          "prefabCameraRefFrames",
          prefab->mDocumentPath,
          refFrameVecName,
          axisName
        };

        Errors ignored;
        settings->SetNumber( root, settingsPath, refFrameVec[ iAxis ], ignored );
      }
    }
  }
}

