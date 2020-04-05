
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
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationGameWindow.h"
#include "src/creation/tacCreationPropertyWindow.h"
#include "src/creation/tacCreationMainWindow.h"
#include "src/creation/tacCreationSystemWindow.h"
#include "src/creation/tacCreationProfileWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"
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
const static String prefabSettingsPath = "prefabs";
static v4 GetClearColor( Shell* shell )
{
  return v4( 1, 0, 0, 1 );
  float visualStudioBackground = 45 / 255.0f;
  visualStudioBackground += 0.3f;
  return GetColorSchemeA( ( float )shell->mElapsedSeconds );
}
const static String refFrameVecNames[] = {
  "mPos",
  "mForwards",
  "mRight",
  "mUp",
};
const static String axisNames[] = { "x", "y", "z" };


void ExecutableStartupInfo::Init( Errors& errors )
{
  mAppName = "Creation";
  mStudioName = "Sleeping Studio";
  new Creation;
}

Creation* Creation::Instance = nullptr;
Creation::Creation()
{
  Instance = this;
}
Creation::~Creation()
{
  if( mMainWindow )
  {
    mMainWindow->mDesktopWindow->mOnDestroyed.clear();
    delete mMainWindow;
  }
  if( mGameWindow )
  {
    mGameWindow->mDesktopWindow->mOnDestroyed.clear();
    delete mGameWindow;
  }
  if( mPropertyWindow )
  {
    mPropertyWindow->mDesktopWindow->mOnDestroyed.clear();
    delete mPropertyWindow;
  }
}

void Creation::CreatePropertyWindow( Errors& errors )
{
  if( mPropertyWindow )
    return;

  Shell* shell = Shell::Instance;
  DesktopWindow* desktopWindow;
  CreateDesktopWindow( gPropertyWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  mPropertyWindow = new CreationPropertyWindow;
  mPropertyWindow->mCreation = this;
  mPropertyWindow->mDesktopWindow = desktopWindow;
  mPropertyWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( []( DesktopWindow* )
                                                     {
                                                       delete Creation::Instance->mPropertyWindow;
                                                       Creation::Instance->mPropertyWindow = nullptr;
                                                     } );
}
void Creation::CreateGameWindow( Errors& errors )
{
  if( mGameWindow )
    return;

  Shell* shell = Shell::Instance;
  DesktopWindow* desktopWindow;
  CreateDesktopWindow( gGameWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  TAC_ASSERT( !mGameWindow );
  mGameWindow = new CreationGameWindow();
  mGameWindow->mCreation = this;
  mGameWindow->mDesktopWindow = desktopWindow;
  mGameWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( []( DesktopWindow* )
                                                     {
                                                       delete Creation::Instance->mGameWindow;
                                                       Creation::Instance->mGameWindow = nullptr;
                                                     } );
}
void Creation::CreateMainWindow( Errors& errors )
{
  if( mMainWindow )
    return;

  Shell* shell = Shell::Instance;
  DesktopWindow* desktopWindow = nullptr;
  CreateDesktopWindow( gMainWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  mMainWindow = new CreationMainWindow();
  mMainWindow->mCreation = this;
  mMainWindow->mDesktopWindow = desktopWindow;
  mMainWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( []( DesktopWindow* )
                                                     {
                                                       // no, for graphics debuggin
                                                       //OS::Instance->mShouldStopRunning = true;

                                                       delete Creation::Instance->mMainWindow;
                                                       Creation::Instance->mMainWindow = nullptr;
                                                     } );
}
void Creation::CreateSystemWindow( Errors& errors )
{
  if( mSystemWindow )
    return;

  Shell* shell = Shell::Instance;
  DesktopWindow* desktopWindow;
  CreateDesktopWindow( gSystemWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  mSystemWindow = new CreationSystemWindow();
  mSystemWindow->mCreation = this;
  mSystemWindow->mDesktopWindow = desktopWindow;
  mSystemWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( []( DesktopWindow* )
                                                     {
                                                       delete Creation::Instance->mSystemWindow;
                                                       Creation::Instance->mSystemWindow = nullptr;
                                                     } );
}

void Creation::CreateProfileWindow( Errors& errors )
{
  if( mProfileWindow )
    return;

  Shell* shell = Shell::Instance;
  DesktopWindow* desktopWindow;
  CreateDesktopWindow( gProfileWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  mProfileWindow = new CreationProfileWindow();
  mProfileWindow->mCreation = this;
  mProfileWindow->mDesktopWindow = desktopWindow;
  mProfileWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( []( DesktopWindow* )
                                                     {
                                                       delete Creation::Instance->mProfileWindow;
                                                       Creation::Instance->mProfileWindow = nullptr;
                                                     } );
}

void Creation::GetWindowsJson( Json** outJson, Errors& errors )
{
  Settings* settings = Shell::Instance->mSettings;
  Vector< String > settingsPaths = { "Windows" };
  auto windowDefault = new Json();
  ( *windowDefault )[ "Name" ] = gMainWindowName;

  auto windowsDefault = new Json();
  windowsDefault->mType = JsonType::Array;
  windowsDefault->mElements.push_back( windowDefault );
  Json* windows = settings->GetArray( nullptr, { "Windows" }, windowsDefault, errors );
  TAC_HANDLE_ERROR( errors );

  *outJson = windows;
}
void Creation::CreateDesktopWindow(
  String windowName,
  DesktopWindow** outDesktopWindow,
  Errors& errors )
{
  Shell* shell = Shell::Instance;
  Settings* settings = shell->mSettings;
  Json* windows;
  GetWindowsJson( &windows, errors );
  TAC_HANDLE_ERROR( errors );


  Json* settingsWindowJson = nullptr;
  for( Json* windowJson : windows->mElements )
  {
    String curWindowName = windowJson->mChildren[ "Name" ]->mString;
    if( curWindowName == windowName )
    {
      settingsWindowJson = windowJson;
      break;
    }
  }

  if( !settingsWindowJson )
  {
    settingsWindowJson = new Json;
    ( *settingsWindowJson )[ "Name" ] = windowName;
    windows->mElements.push_back( settingsWindowJson );
  }

  Json* windowJson = settingsWindowJson;

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
    Monitor monitor;
    DesktopApp::Instance->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );
    WindowParams::GetCenteredPosition(
      width,
      height,
      &x,
      &y,
      monitor );
  }

  WindowParams windowParams = {};
  windowParams.mName = windowName;
  windowParams.mX = x;
  windowParams.mY = y;
  windowParams.mWidth = width;
  windowParams.mHeight = height;

  DesktopWindow* desktopWindow;
  DesktopApp::Instance->SpawnWindow( windowParams, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );


  desktopWindow->mOnResize.AddCallbackFunctional( [ windowJson, settings, desktopWindow, &errors ]()
                                                  {
                                                    windowJson->operator[]( "w" ) = desktopWindow->mWidth;
                                                    windowJson->operator[]( "h" ) = desktopWindow->mHeight;
                                                    settings->Save( errors );
                                                  } );

  desktopWindow->mOnMove.AddCallbackFunctional( [ windowJson, settings, desktopWindow, &errors ]()
                                                {
                                                  windowJson->operator[]( "x" ) = desktopWindow->mX;
                                                  windowJson->operator[]( "y" ) = desktopWindow->mY;
                                                  settings->Save( errors );
                                                } );

  *outDesktopWindow = desktopWindow;
}

bool Creation::ShouldCreateWindowNamed( StringView name )
{
  if( mOnlyCreateWindowNamed.size() && name != mOnlyCreateWindowNamed )
    return false;
  Json* windowJson = FindWindowJson( name);
  if( !windowJson )
    return false;

  Settings* settings = Shell::Instance->mSettings;
  Errors errors;
  const bool create = settings->GetBool( windowJson, { "Create" }, false, errors );
  if( errors )
    return false;
  return create;
}

void Creation::SetSavedWindowData( Json* windowJson, Errors& errors )
{
  Settings* settings = Shell::Instance->mSettings;

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
    Monitor monitor;
    DesktopApp::Instance->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );
    WindowParams::GetCenteredPosition(
      width,
      height,
      &x,
      &y,
      monitor );
  }

  DesktopWindowManager::Instance->SetWindowCreationData( name, width, height, x, y );
}
void Creation::SetSavedWindowsData( Errors& errors )
{
  Settings* settings = Shell::Instance->mSettings;

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
  OS* os = OS::Instance;

  SpaceInit();
  mWorld = new World;
  mEditorCamera.mPos = { 0, 1, 5 };
  mEditorCamera.mForwards = { 0, 0, -1 };
  mEditorCamera.mRight = { 1, 0, 0 };
  mEditorCamera.mUp = { 0, 1, 0 };

  String dataPath;
  os->GetApplicationDataPath( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  os->CreateFolderIfNotExist( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  Shell* shell = Shell::Instance;
  Settings* settings = shell->mSettings;

  SetSavedWindowsData( errors );
  TAC_HANDLE_ERROR( errors );

  Json* windows;
  GetWindowsJson( &windows, errors );
  TAC_HANDLE_ERROR( errors );

  mOnlyCreateWindowNamed = settings->GetString(
    nullptr, { "onlyCreateWindowNamed" }, "", errors );
  TAC_HANDLE_ERROR( errors );

  // The first window spawned becomes the parent window
  if( ShouldCreateWindowNamed( gMainWindowName) )
    CreateMainWindow( errors );
  TAC_HANDLE_ERROR( errors );

  if( ShouldCreateWindowNamed( gPropertyWindowName) )
    CreatePropertyWindow( errors );
  TAC_HANDLE_ERROR( errors );

  if( ShouldCreateWindowNamed( gGameWindowName) )
    CreateGameWindow( errors );
  TAC_HANDLE_ERROR( errors );

  if( ShouldCreateWindowNamed( gSystemWindowName) )
    CreateSystemWindow( errors );
  TAC_HANDLE_ERROR( errors );

  if( ShouldCreateWindowNamed( gProfileWindowName) )
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
  if(errors)
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
  Shell* shell = Shell::Instance;
  if( mMainWindow )
  {
    mMainWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( mGameWindow )
  {
    mGameWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( mPropertyWindow )
  {
    mPropertyWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( mSystemWindow )
  {
    mSystemWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( mProfileWindow )
  {
    mProfileWindow->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  mWorld->Step( TAC_DELTA_FRAME_SECONDS );

  if( KeyboardInput::Instance->IsKeyJustDown( Key::Delete ) &&
      mGameWindow->mDesktopWindow->mCursorUnobscured )
  {
    DeleteSelectedEntities();
  }

  if( KeyboardInput::Instance->IsKeyJustDown( Key::S ) &&
      KeyboardInput::Instance->IsKeyDown( Key::Modifier ) )
  {
    SavePrefabs();
    if( mGameWindow )
    {
      mGameWindow->mStatusMessage = "Saved prefabs!";
      mGameWindow->mStatusMessageEndTime = shell->mElapsedSeconds + 5.0f;
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
    bool isEntityNameUnique = false;
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
void Creation::GetSavedPrefabs( Vector< String > & paths, Errors& errors )
{
  Shell* shell = Shell::Instance;
  Settings* settings = shell->mSettings;
  Json& prefabs = settings->mJson[ prefabSettingsPath ];
  prefabs.mType = JsonType::Array;

  Vector< String > alreadySavedPrefabs;
  for( Json* child : prefabs.mElements )
    alreadySavedPrefabs.push_back( child->mString );
  paths = alreadySavedPrefabs;
}
void Creation::UpdateSavedPrefabs()
{
  Shell* shell = Shell::Instance;
  Settings* settings = shell->mSettings;
  Json& prefabs = settings->mJson[ prefabSettingsPath ];
  prefabs.mType = JsonType::Array;

  Errors errors;
  Vector< String > alreadySavedPrefabs;
  GetSavedPrefabs( alreadySavedPrefabs, errors );
  TAC_HANDLE_ERROR( errors );

  for( Prefab* prefab : mPrefabs )
  {
    const String& path = prefab->mDocumentPath;
    if( path.empty() )
      continue;
    if( Contains( alreadySavedPrefabs, path ) )
      continue;
    prefabs.mElements.push_back( new Json( path ) );
  }

  settings->Save( errors );
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
  Shell* shell = Shell::Instance;
  OS* os = OS::Instance;

  for( Entity* entity : mWorld->mEntities )
  {
    if( entity->mParent )
      continue;

    Prefab* prefab = FindPrefab( entity );
    if( !prefab )
    {
      prefab = new Prefab;
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
      os->SaveDialog( savePath, suggestedName, saveDialogErrors );
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
    os->SaveToFile( prefab->mDocumentPath, bytes, byteCount, saveToFileErrors );
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
  Shell* shell = Shell::Instance;
  if( StartsWith( savePath, shell->mInitialWorkingDir ) )
  {
    savePath = savePath.substr( shell->mInitialWorkingDir.size() );
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

  auto prefab = new Prefab;
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
  for( const String& prefabPath : prefabPaths )
  {
    LoadPrefabAtPath( prefabPath, errors );
    TAC_HANDLE_ERROR( errors );
  }
}
void Creation::LoadPrefabCameraPosition( Prefab* prefab )
{
  if( prefab->mDocumentPath.empty() )
    return;
  Settings* settings = Shell::Instance->mSettings;
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
    String refFrameVecName = refFrameVecNames[ iRefFrameVec ];
    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      const String& axisName = axisNames[ iAxis ];

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
  Settings* settings = Shell::Instance->mSettings;

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
      const String& axisName = axisNames[ iAxis ];

      Vector< String > settingsPath = {
        "prefabCameraRefFrames",
        prefab->mDocumentPath,
        refFrameVecName,
        axisName };

      Errors ignored;
      settings->SetNumber( root, settingsPath, refFrameVec[ iAxis ], ignored );
    }
  }
}

void SetCreationWindowImGuiGlobals(
  DesktopWindow* desktopWindow,
  UI2DDrawData* ui2DDrawData )
{
  Errors screenspaceCursorPosErrors;
  v2 screenspaceCursorPos = {};
  OS::Instance->GetScreenspaceCursorPos( screenspaceCursorPos, screenspaceCursorPosErrors );
  bool isWindowDirectlyUnderCursor = false;
  v2 mousePositionDestopWindowspace = {};
  if( screenspaceCursorPosErrors.empty() )
  {
    mousePositionDestopWindowspace = {
      screenspaceCursorPos.x - desktopWindow->mX,
      screenspaceCursorPos.y - desktopWindow->mY };
    isWindowDirectlyUnderCursor = desktopWindow->mCursorUnobscured;
  }

  ImGuiSetGlobals(
    mousePositionDestopWindowspace,
    isWindowDirectlyUnderCursor,
    Shell::Instance->mElapsedSeconds,
    ui2DDrawData );
}

}

