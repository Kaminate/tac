#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/graphics/tacColorUtil.h"
#include "common/graphics/tacFont.h"
#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacUI2D.h"
#include "common/math/tacMath.h"
#include "common/tacAlgorithm.h"
#include "common/tacTime.h"
#include "common/tacOS.h"
#include "common/tacPreprocessor.h"
#include "common/tackeyboardinput.h"
#include "creation/tacCreation.h"
#include "creation/tacCreationGameWindow.h"
#include "creation/tacCreationPropertyWindow.h"
#include "creation/tacCreationMainWindow.h"
#include "creation/tacCreationSystemWindow.h"
#include "shell/tacDesktopApp.h"
#include "space/tacGhost.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/model/tacmodel.h"
#include "space/terrain/tacterrain.h"
#include "space/tacspace.h"

#include <iostream>
#include <functional>
#include <algorithm>

const static TacString prefabSettingsPath = "prefabs";
static v4 GetClearColor( TacShell* shell )
{
  return v4( 1, 0, 0, 1 );
  float visualStudioBackground = 45 / 255.0f;
  visualStudioBackground += 0.3f;
  return TacGetColorSchemeA( ( float )shell->mElapsedSeconds );
}
const static TacString refFrameVecNames[] = {
  "mPos",
  "mForwards",
  "mRight",
  "mUp",
};
const static TacString axisNames[] = { "x", "y", "z" };

void TacDesktopApp::DoStuff( TacDesktopApp* desktopApp, TacErrors& errors )
{
  TacOS* os = TacOS::Instance;
  TacString appDataPath;
  os->GetApplicationDataPath( appDataPath, errors );

  TacString appName = "Creation";
  TacString studioPath = appDataPath + "\\Sleeping Studio\\";
  TacString prefPath = studioPath + appName;

  bool appDataPathExists;
  os->DoesFolderExist( appDataPath, appDataPathExists, errors );
  TacAssert( appDataPathExists );

  os->CreateFolderIfNotExist( studioPath, errors );
  TAC_HANDLE_ERROR( errors );

  os->CreateFolderIfNotExist( prefPath, errors );
  TAC_HANDLE_ERROR( errors );


  TacString workingDir;
  os->GetWorkingDir( workingDir, errors );
  TAC_HANDLE_ERROR( errors );

  TacShell* shell = desktopApp->mShell;
  shell->mAppName = appName;
  shell->mPrefPath = prefPath;
  shell->mInitialWorkingDir = workingDir;
  shell->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->OnShellInit( errors );
  TAC_HANDLE_ERROR( errors );

  auto creation = new TacCreation();
  creation->mDesktopApp = desktopApp;
  shell->mOnUpdate.AddCallbackFunctional( [ creation, &errors ]()
    {
      creation->Update( errors );
    } );

  creation->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->Loop( errors );
  TAC_HANDLE_ERROR( errors );

  delete creation;
}

TacCreation::~TacCreation()
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

void TacCreation::CreatePropertyWindow( TacErrors& errors )
{
  if( mPropertyWindow )
    return;

  TacShell* shell = mDesktopApp->mShell;
  TacDesktopWindow* desktopWindow;
  CreateDesktopWindow( gPropertyWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  mPropertyWindow = new TacCreationPropertyWindow;
  mPropertyWindow->mCreation = this;
  mPropertyWindow->mDesktopWindow = desktopWindow;
  mPropertyWindow->mShell = shell;
  mPropertyWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( [ this ]()
    {
      delete mPropertyWindow;
      mPropertyWindow = nullptr;
    } );
}
void TacCreation::CreateGameWindow( TacErrors& errors )
{
  if(mGameWindow)
    return;

  TacShell* shell = mDesktopApp->mShell;
  TacDesktopWindow* desktopWindow;
  CreateDesktopWindow( gGameWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  TacAssert( !mGameWindow );
  mGameWindow = new TacCreationGameWindow();
  mGameWindow->mCreation = this;
  mGameWindow->mDesktopWindow = desktopWindow;
  mGameWindow->mShell = shell;
  mGameWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( [ this ]()
    {
      delete mGameWindow;
      mGameWindow = nullptr;
    } );
}
void TacCreation::CreateMainWindow( TacErrors& errors )
{
  if( mMainWindow )
    return;

  TacShell* shell = mDesktopApp->mShell;
  TacDesktopWindow* desktopWindow = nullptr;
  CreateDesktopWindow( gMainWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  mMainWindow = new TacCreationMainWindow();
  mMainWindow->mCreation = this;
  mMainWindow->mDesktopWindow = desktopWindow;
  mMainWindow->mDesktopApp = mDesktopApp;
  mMainWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( [ this ]()
    {
      TacOS::Instance->mShouldStopRunning = true;
      delete mMainWindow;
      mMainWindow = nullptr;
    } );
}
void TacCreation::CreateSystemWindow( TacErrors& errors )
{
  if( mSystemWindow )
    return;

  TacShell* shell = mDesktopApp->mShell;
  TacDesktopWindow* desktopWindow;
  CreateDesktopWindow( gSystemWindowName, &desktopWindow, errors );
  TAC_HANDLE_ERROR( errors );

  mSystemWindow = new TacCreationSystemWindow();
  mSystemWindow->mCreation = this;
  mSystemWindow->mDesktopWindow = desktopWindow;
  mSystemWindow->mShell = shell;
  mSystemWindow->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopWindow->mOnDestroyed.AddCallbackFunctional( [ this ]()
    {
      delete mSystemWindow;
      mSystemWindow = nullptr;
    } );
}

void TacCreation::GetWindowsJson( TacJson** outJson, TacErrors& errors )
{
  TacShell* shell = mDesktopApp->mShell;
  TacSettings* settings = shell->mSettings;
  TacVector< TacString > settingsPaths = { "Windows" };
  auto windowDefault = new TacJson();
  ( *windowDefault )[ "Name" ] = gMainWindowName;

  auto windowsDefault = new TacJson();
  windowsDefault->mType = TacJsonType::Array;
  windowsDefault->mElements.push_back( windowDefault );
  TacJson* windows = settings->GetArray( nullptr, { "Windows" }, windowsDefault, errors );
  TAC_HANDLE_ERROR( errors );

  *outJson = windows;
}
void TacCreation::CreateDesktopWindow(
  TacString windowName,
  TacDesktopWindow** outDesktopWindow,
  TacErrors& errors )
{
  TacShell* shell = mDesktopApp->mShell;
  TacSettings* settings = shell->mSettings;
  TacJson* windows;
  GetWindowsJson( &windows, errors );
  TAC_HANDLE_ERROR( errors );


  TacJson* settingsWindowJson = nullptr;
  for( TacJson* windowJson : windows->mElements )
  {
    TacString curWindowName = windowJson->mChildren[ "Name" ]->mString;
    if( curWindowName == windowName )
    {
      settingsWindowJson = windowJson;
      break;
    }
  }

  if( !settingsWindowJson )
  {
    settingsWindowJson = new TacJson;
    ( *settingsWindowJson )[ "Name" ] = windowName;
    windows->mElements.push_back( settingsWindowJson );
  }

  TacJson* windowJson = settingsWindowJson;

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
    TacMonitor monitor;
    mDesktopApp->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );
    TacWindowParams::GetCenteredPosition(
      width,
      height,
      &x,
      &y,
      monitor );
  }

  TacWindowParams windowParams = {};
  windowParams.mName = windowName;
  windowParams.mX = x;
  windowParams.mY = y;
  windowParams.mWidth = width;
  windowParams.mHeight = height;

  TacDesktopWindow* desktopWindow;
  mDesktopApp->SpawnWindow( windowParams, &desktopWindow, errors );
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

bool TacCreation::HasWindowNamed( TacJson* windows, const TacString& name )
{
  for( TacJson* windowJson : windows->mElements )
  {
    TacJson& nameJson = ( *windowJson )[ "Name" ];
    TacString curName = nameJson;

    curName = ( ( *windowJson )[ "Name" ] );

    if( curName == name )
      return true;
  }
  return false;
}
void TacCreation::Init( TacErrors& errors )
{
  TacOS* os = TacOS::Instance;

  TacSpaceInit();
  mWorld = new TacWorld;
  mEditorCamera.mPos = { 0, 1, 5 };
  mEditorCamera.mForwards = { 0, 0, -1 };
  mEditorCamera.mRight = { 1, 0, 0 };
  mEditorCamera.mUp = { 0, 1, 0 };

  TacString dataPath;
  os->GetApplicationDataPath( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  os->CreateFolderIfNotExist( dataPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacShell* shell = mDesktopApp->mShell;
  TacSettings* settings = shell->mSettings;

  TacJson* windows;
  GetWindowsJson( &windows, errors );
  TAC_HANDLE_ERROR( errors );

  if( HasWindowNamed( windows, gMainWindowName ) )
  {
    CreateMainWindow( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( HasWindowNamed( windows, gPropertyWindowName ) )
  {
    CreatePropertyWindow( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( HasWindowNamed( windows, gGameWindowName ) )
  {
    CreateGameWindow( errors );
    TAC_HANDLE_ERROR( errors );
  }

  if( HasWindowNamed( windows, gSystemWindowName ) )
  {
    CreateSystemWindow( errors );
    TAC_HANDLE_ERROR( errors );
  }

  LoadPrefabs( errors );
  TAC_HANDLE_ERROR( errors );
}
void TacCreation::RemoveEntityFromPrefabRecursively( TacEntity* entity )
{
  int prefabCount = mPrefabs.size();
  for( int iPrefab = 0; iPrefab < prefabCount; ++iPrefab )
  {
    TacPrefab* prefab = mPrefabs[ iPrefab ];
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
  for( TacEntity* child : entity->mChildren )
    RemoveEntityFromPrefabRecursively( child );
}
void TacCreation::DeleteSelectedEntities()
{
  TacVector< TacEntity* > topLevelEntitiesToDelete;

  for( TacEntity* entity : mSelectedEntities )
  {
    bool isTopLevel = true;
    for( TacEntity* parent = entity->mParent; parent; parent = parent->mParent )
    {
      if( TacContains( mSelectedEntities, parent ) )
      {
        isTopLevel = false;
        break;
      }
    }
    if( isTopLevel )
      topLevelEntitiesToDelete.push_back( entity );
  }
  for( TacEntity* entity : topLevelEntitiesToDelete )
  {
    RemoveEntityFromPrefabRecursively( entity );
    mWorld->KillEntity( entity );
  }
  mSelectedEntities.clear();
}
void TacCreation::Update( TacErrors& errors )
{
  TacShell* shell = mDesktopApp->mShell;
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

  mWorld->Step( TAC_DELTA_FRAME_SECONDS );

  TacKeyboardInput* keyboardInput = shell->mKeyboardInput;
  if( keyboardInput->IsKeyJustDown( TacKey::Delete ) &&
    mGameWindow->mDesktopWindow->mCursorUnobscured )
  {
    DeleteSelectedEntities();
  }

  if( keyboardInput->IsKeyJustDown( TacKey::S ) &&
    keyboardInput->IsKeyDown( TacKey::Modifier ) )
  {
    SavePrefabs();
  }
}
TacEntity* TacCreation::CreateEntity()
{
  TacWorld* world = mWorld;
  TacString desiredEntityName = "Entity";
  int parenNumber = 1;
  for( ;; )
  {
    bool isEntityNameUnique = false;
    TacEntity* entity = world->FindEntity( desiredEntityName );
    if( !entity )
      break;
    desiredEntityName = "Entity (" + TacToString( parenNumber ) + ")";
    parenNumber++;
  }

  TacEntity* entity = world->SpawnEntity( TacNullEntityUUID );
  entity->mName = desiredEntityName;
  mSelectedEntities = { entity };
  return entity;
}
bool TacCreation::IsAnythingSelected()
{
  return mSelectedEntities.size();
}
v3 TacCreation::GetSelectionGizmoOrigin()
{
  TacAssert( IsAnythingSelected() );
  // do i really want average? or like center of bounding circle?
  v3 runningPosSum = {};
  int selectionCount = 0;
  for( auto entity : mSelectedEntities )
  {
    runningPosSum +=
      ( entity->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
    entity->mLocalPosition;
    selectionCount++;
  }
  v3 averagePos = runningPosSum / ( float )selectionCount;
  v3 result = averagePos;
  if( mSelectedHitOffsetExists )
    result += mSelectedHitOffset;
  return result;
}
void TacCreation::ClearSelection()
{
  mSelectedEntities.clear();
  mSelectedHitOffsetExists = false;
}
void TacCreation::GetSavedPrefabs( TacVector< TacString > & paths, TacErrors& errors )
{
  TacShell* shell = mDesktopApp->mShell;
  TacSettings* settings = shell->mSettings;
  TacJson& prefabs = settings->mJson[ prefabSettingsPath ];
  prefabs.mType = TacJsonType::Array;

  TacVector< TacString > alreadySavedPrefabs;
  for( TacJson* child : prefabs.mElements )
    alreadySavedPrefabs.push_back( child->mString );
  paths = alreadySavedPrefabs;
}
void TacCreation::UpdateSavedPrefabs()
{
  TacShell* shell = mDesktopApp->mShell;
  TacSettings* settings = shell->mSettings;
  TacJson& prefabs = settings->mJson[ prefabSettingsPath ];
  prefabs.mType = TacJsonType::Array;

  TacErrors errors;
  TacVector< TacString > alreadySavedPrefabs;
  GetSavedPrefabs( alreadySavedPrefabs, errors );
  TAC_HANDLE_ERROR( errors );

  for( TacPrefab* prefab : mPrefabs )
  {
    const TacString& path = prefab->mDocumentPath;
    if( path.empty() )
      continue;
    if( TacContains( alreadySavedPrefabs, path ) )
      continue;
    prefabs.mElements.push_back( new TacJson( path ) );
  }

  settings->Save( errors );
}
TacPrefab* TacCreation::FindPrefab( TacEntity* entity )
{
  for( TacPrefab* prefab : mPrefabs )
  {
    if( TacContains( prefab->mEntities, entity ) )
    {
      return prefab;
    }
  }
  return nullptr;
}
void TacCreation::SavePrefabs()
{
  TacShell* shell = mDesktopApp->mShell;
  TacOS* os = TacOS::Instance;

  for( TacEntity* entity : mWorld->mEntities )
  {
    if( entity->mParent )
      continue;

    TacPrefab* prefab = FindPrefab( entity );
    if( !prefab )
    {
      prefab = new TacPrefab;
      prefab->mEntities = { entity };
      mPrefabs.push_back( prefab );
    }


    // Get document paths for prefabs missing them
    if( prefab->mDocumentPath.empty() )
    {
      TacString savePath;
      TacString suggestedName =
        //prefab->mEntity->mName
        entity->mName +
        ".prefab";
      TacErrors saveDialogErrors;
      os->SaveDialog( savePath, suggestedName, saveDialogErrors );
      if( saveDialogErrors.size() )
      {
        // todo: log it, user feedback
        std::cout << saveDialogErrors.ToString() << std::endl;
        continue;
      }

      ModifyPathRelative( savePath );

      prefab->mDocumentPath = savePath;
      UpdateSavedPrefabs();
    }

    //TacEntity* entity = prefab->mEntity;

    TacJson entityJson;
    entity->Save( entityJson );

    TacString prefabJsonString = entityJson.Stringify();
    TacErrors saveToFileErrors;
    void* bytes = prefabJsonString.data();
    int byteCount = prefabJsonString.size();
    os->SaveToFile( prefab->mDocumentPath, bytes, byteCount, saveToFileErrors );
    if( saveToFileErrors.size() )
    {
      // todo: log it, user feedback
      std::cout << saveToFileErrors.ToString() << std::endl;
      continue;
    }
  }
}
void TacCreation::ModifyPathRelative( TacString& savePath )
{
  TacShell* shell = mDesktopApp->mShell;
  if( TacStartsWith( savePath, shell->mInitialWorkingDir ) )
  {
    savePath = savePath.substr( shell->mInitialWorkingDir.size() );
    savePath = TacStripLeadingSlashes( savePath );
  }
}
void TacCreation::LoadPrefabAtPath( TacString prefabPath, TacErrors& errors )
{
  ModifyPathRelative( prefabPath );
  auto memory = TacTemporaryMemory( prefabPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacJson prefabJson;
  prefabJson.Parse( memory.data(), memory.size(), errors );

  TacEntity* entity = mWorld->SpawnEntity( TacNullEntityUUID );
  entity->Load( prefabJson );

  auto prefab = new TacPrefab;
  prefab->mDocumentPath = prefabPath;
  prefab->mEntities = { entity };
  mPrefabs.push_back( prefab );

  LoadPrefabCameraPosition( prefab );
  UpdateSavedPrefabs();
}
void TacCreation::LoadPrefabs( TacErrors& errors )
{
  TacVector< TacString > prefabPaths;
  GetSavedPrefabs( prefabPaths, errors );
  for( const TacString& prefabPath : prefabPaths )
  {
    LoadPrefabAtPath( prefabPath, errors );
    TAC_HANDLE_ERROR( errors );
  }
}
void TacCreation::LoadPrefabCameraPosition( TacPrefab* prefab )
{
  if( prefab->mDocumentPath.empty() )
    return;
  TacSettings* settings = mDesktopApp->mShell->mSettings;
  TacJson* root = nullptr;
  v3* refFrameVecs[] = {
    &mEditorCamera.mPos,
    &mEditorCamera.mForwards,
    &mEditorCamera.mRight,
    &mEditorCamera.mUp,
  };
  for( int iRefFrameVec = 0; iRefFrameVec < 4; ++iRefFrameVec )
  {
    v3* refFrameVec = refFrameVecs[ iRefFrameVec ];
    TacString refFrameVecName = refFrameVecNames[ iRefFrameVec ];
    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      const TacString& axisName = axisNames[ iAxis ];

      TacVector< TacString > settingsPath = {
        "prefabCameraRefFrames",
        prefab->mDocumentPath,
        refFrameVecName,
        axisName };

      TacErrors ignored;
      TacJsonNumber defaultValue = refFrameVecs[ iRefFrameVec ]->operator[]( iAxis );
      TacJsonNumber axisValue = settings->GetNumber(
        root,
        settingsPath,
        defaultValue,
        ignored );

      refFrameVec->operator[]( iAxis ) = ( float )axisValue;
    }
  }
}
void TacCreation::SavePrefabCameraPosition( TacPrefab* prefab )
{
  if( prefab->mDocumentPath.empty() )
    return;
  TacJson* root = nullptr;
  TacSettings* settings = mDesktopApp->mShell->mSettings;

  v3 refFrameVecs[] = {
    mEditorCamera.mPos,
    mEditorCamera.mForwards,
    mEditorCamera.mRight,
    mEditorCamera.mUp,
  };
  for( int iRefFrameVec = 0; iRefFrameVec < 4; ++iRefFrameVec )
  {
    v3 refFrameVec = refFrameVecs[ iRefFrameVec ];
    TacString refFrameVecName = refFrameVecNames[ iRefFrameVec ];
    for( int iAxis = 0; iAxis < 3; ++iAxis )
    {
      const TacString& axisName = axisNames[ iAxis ];

      TacVector< TacString > settingsPath = {
        "prefabCameraRefFrames",
        prefab->mDocumentPath,
        refFrameVecName,
        axisName };

      TacErrors ignored;
      settings->SetNumber( root, settingsPath, refFrameVec[ iAxis ], ignored );
    }
  }
}
