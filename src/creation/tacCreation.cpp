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
#include "shell/tacDesktopApp.h"
#include "space/tacGhost.h"
#include "space/tacentity.h"
#include "space/tacworld.h"
#include "space/tacmodel.h"

#include <iostream>
#include <functional>
#include <algorithm>

const TacString prefabSettingsPath = "prefabs";
static v4 GetClearColor( TacShell* shell )
{
  return v4( 1, 0, 0, 1 );
  float visualStudioBackground = 45 / 255.0f;
  visualStudioBackground += 0.3f;
  return TacGetColorSchemeA( ( float )shell->mElapsedSeconds );
}



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
void TacCreation::Init( TacErrors& errors )
{
  TacOS* os = TacOS::Instance;
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

  TacVector< TacString > settingsPaths = { "Windows" };
  auto windowsDefault = new TacJson();
  windowsDefault->mType = TacJsonType::Array;
  windowsDefault->mElements.push_back( new TacJson() );
  TacJson* windows = settings->GetArray( nullptr, { "Windows" }, windowsDefault, errors );
  TAC_HANDLE_ERROR( errors );

  for( TacJson* windowJson : windows->mElements )
  {
    bool shouldCreate = settings->GetBool( windowJson, { "Create" }, true, errors );
    TAC_HANDLE_ERROR( errors );

    if( !shouldCreate )
      continue;


    TacWindowParams windowParams = {};
    windowParams.mName = settings->GetString( windowJson, { "Name" }, "unnamed window", errors );

    TacMonitor monitor;
    mDesktopApp->GetPrimaryMonitor( &monitor, errors );
    TAC_HANDLE_ERROR( errors );

    windowParams.mWidth = ( int )settings->GetNumber( windowJson, { "w" }, 800, errors );
    TAC_HANDLE_ERROR( errors );

    windowParams.mHeight = ( int )settings->GetNumber( windowJson, { "h" }, 600, errors );
    TAC_HANDLE_ERROR( errors );

    bool centered = ( int )settings->GetBool( windowJson, { "centered" }, false, errors );
    TAC_HANDLE_ERROR( errors );

    if( centered )
    {
      TacWindowParams::GetCenteredPosition(
        windowParams.mWidth,
        windowParams.mHeight,
        &windowParams.mX,
        &windowParams.mY,
        monitor );
    }
    else
    {
      windowParams.mX = ( int )settings->GetNumber( windowJson, { "x" }, 50, errors );
      TAC_HANDLE_ERROR( errors );

      windowParams.mY = ( int )settings->GetNumber( windowJson, { "y" }, 50, errors );
      TAC_HANDLE_ERROR( errors );
    }
    TAC_HANDLE_ERROR( errors );

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

    //auto ui2DDrawData = new TacUI2DDrawData();
    //ui2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
    //ui2DDrawData->mRenderView = desktopWindow->mMainWindowRenderView;

    //auto uiRoot = new TacUIRoot();
    //uiRoot->mKeyboardInput = shell->mKeyboardInput;
    //uiRoot->mElapsedSeconds = &shell->mElapsedSeconds;
    //uiRoot->mUI2DDrawData = ui2DDrawData;

    //auto editorWindow = new TacEditorWindow();
    //editorWindow->mCreation = this;
    //editorWindow->mDesktopWindow = desktopWindow;
    //editorWindow->mUI2DDrawData = ui2DDrawData;
    //editorWindow->mUIRoot = uiRoot;

    if( windowParams.mName == gMainWindowName )
    {
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

    if( windowParams.mName == gGameWindowName )
    {
      TacAssert( !mGameWindow );
      mGameWindow = new TacCreationGameWindow();
      mGameWindow->mCreation = this;
      mGameWindow->mShell = shell;
      mGameWindow->mDesktopWindow = desktopWindow;
      mGameWindow->Init( errors );
      TAC_HANDLE_ERROR( errors );

      desktopWindow->mOnDestroyed.AddCallbackFunctional( [ this ]()
        {
          delete mGameWindow;
          mGameWindow = nullptr;
        } );
    }

    if( windowParams.mName == gPropertyWindowName )
    {
      mPropertyWindow = new TacCreationPropertyWindow;
      mPropertyWindow->mShell = shell;
      mPropertyWindow->mCreation = this;
      mPropertyWindow->mDesktopWindow = desktopWindow;
      mPropertyWindow->Init( errors );
      TAC_HANDLE_ERROR( errors );

      desktopWindow->mOnDestroyed.AddCallbackFunctional( [ this ]()
        {
          delete mPropertyWindow;
          mPropertyWindow = nullptr;
        } );
    }
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
TacJson TacCreation::SaveEntityToJsonRecusively( TacEntity* entity )
{
  TacJson entityJson;
  {
    TacJson posJson;
    {
      v3 position = entity->mLocalPosition;
      posJson[ "x" ] = position.x;
      posJson[ "y" ] = position.y;
      posJson[ "z" ] = position.z;
    }

    TacJson scaleJson;
    {
      v3 scale = entity->mLocalScale;
      scaleJson[ "x" ] = scale.x;
      scaleJson[ "y" ] = scale.y;
      scaleJson[ "z" ] = scale.z;
    }

    TacJson eulerRadsJson;
    {
      eulerRadsJson[ "x" ] = entity->mLocalEulerRads.x;
      eulerRadsJson[ "y" ] = entity->mLocalEulerRads.y;
      eulerRadsJson[ "z" ] = entity->mLocalEulerRads.z;
    }
    entityJson[ "mPosition" ] = posJson;
    entityJson[ "mScale" ] = scaleJson;
    entityJson[ "mName" ] = entity->mName;
    entityJson[ "mEulerRads" ] = eulerRadsJson;
    entityJson[ "mEntityUUID" ] = ( TacJsonNumber )entity->mEntityUUID;

    // todo: GetComponentJsonSerializer( component )->SerializeToJson( ... )
    if( TacModel* model = TacModel::GetModel( entity ) )
    {
      TacJson colorRGBJson;
      colorRGBJson[ "r" ] = model->mColorRGB[ 0 ];
      colorRGBJson[ "g" ] = model->mColorRGB[ 1 ];
      colorRGBJson[ "b" ] = model->mColorRGB[ 2 ];

      TacJson modelJson;
      modelJson[ "mGLTFPath" ] = model->mGLTFPath;
      modelJson[ "mColorRGB" ] = colorRGBJson;

      entityJson[ "Model" ] = modelJson;
    }

    if( !entity->mChildren.empty() )
    {
      TacJson childrenJson;
      childrenJson.mType = TacJsonType::Array;
      for( TacEntity* child : entity->mChildren )
      {
        auto childJson = new TacJson( SaveEntityToJsonRecusively( child ) );
        childrenJson.mElements.push_back( childJson );
      }
      entityJson[ "mChildren" ] = childrenJson;
    }
  }

  return entityJson;
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

      if( TacStartsWith( savePath, shell->mInitialWorkingDir ) )
      {
        savePath = savePath.substr( shell->mInitialWorkingDir.size() );
        savePath = TacStripLeadingSlashes( savePath );
      }

      prefab->mDocumentPath = savePath;
      UpdateSavedPrefabs();
    }

    //TacEntity* entity = prefab->mEntity;

    TacJson entityJson = SaveEntityToJsonRecusively( entity );

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

TacEntity* TacCreation::LoadEntityFromJsonRecursively( TacJson& prefabJson )
{

  TacJson& positionJson = prefabJson[ "mPosition" ];
  v3 pos =
  {
    ( float )positionJson[ "x" ].mNumber,
    ( float )positionJson[ "y" ].mNumber,
    ( float )positionJson[ "z" ].mNumber,
  };

  TacJson& scaleJson = prefabJson[ "mScale" ];
  v3 scale =
  {
    ( float )scaleJson[ "x" ].mNumber,
    ( float )scaleJson[ "y" ].mNumber,
    ( float )scaleJson[ "z" ].mNumber,
  };

  TacJson& eulerRadsJson = prefabJson[ "mEulerRads" ];
  v3 eulerRads =
  {
    ( float )eulerRadsJson[ "x" ].mNumber,
    ( float )eulerRadsJson[ "y" ].mNumber,
    ( float )eulerRadsJson[ "z" ].mNumber,
  };

  TacEntity* entity = CreateEntity();
  entity->mLocalPosition = pos;
  entity->mLocalScale = scale;
  entity->mLocalEulerRads = eulerRads;
  entity->mName = prefabJson[ "mName" ].mString;
  entity->mEntityUUID = ( TacEntityUUID )( TacUUID )prefabJson[ "mEntityUUID" ].mNumber;

  if( TacJson* modelJson = prefabJson.mChildren[ "Model" ] )
  {
    auto model = ( TacModel* )entity->AddNewComponent( TacModel::ComponentRegistryEntry );
    model->mGLTFPath = ( *modelJson )[ "mGLTFPath" ].mString;
    model->mColorRGB = {
      ( float )( *modelJson )[ "mColorRGB" ][ "r" ].mNumber,
      ( float )( *modelJson )[ "mColorRGB" ][ "g" ].mNumber,
      ( float )( *modelJson )[ "mColorRGB" ][ "b" ].mNumber };
  }

  if( TacJson* childrenJson = prefabJson.mChildren[ "mChildren" ] )
  {
    for( TacJson* childJson : childrenJson->mElements )
    {
      TacEntity* childEntity = LoadEntityFromJsonRecursively( *childJson );
      entity->AddChild( childEntity );
    }
  }

  return entity;
}
void TacCreation::LoadPrefabs( TacErrors& errors )
{
  TacVector< TacString > prefabPaths;
  GetSavedPrefabs( prefabPaths, errors );
  for( TacString prefabPath : prefabPaths )
  {
    auto memory = TacTemporaryMemory( prefabPath, errors );
    TacJson prefabJson;
    prefabJson.Parse( memory.data(), memory.size(), errors );

    TacEntity* entity = LoadEntityFromJsonRecursively( prefabJson );


    TacPrefab* prefab = new TacPrefab;
    prefab->mDocumentPath = prefabPath;
    prefab->mEntities = { entity };
    mPrefabs.push_back( prefab );

    LoadPrefabCameraPosition( prefab );
  }
}

TacString refFrameVecNames[] = {
  "mPos",
  "mForwards",
  "mRight",
  "mUp",
};

TacString axisNames[] = { "x", "y", "z" };

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
      TacJsonNumber defaultValue = 0;
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
