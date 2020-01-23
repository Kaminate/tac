#pragma once

#include "common/tacCamera.h"
#include "common/tacMemory.h"
#include "common/tacErrorHandling.h"
#include "common/tacSettings.h"
#include "common/tacEvent.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacRenderer.h"
#include "common/containers/tacVector.h"


struct TacCreation;
struct TacCreationMainWindow;
struct TacCreationGameWindow;
struct TacCreationPropertyWindow;
struct TacCreationSystemWindow;
struct TacCreationProfileWindow;
struct TacDesktopApp;
struct TacDesktopWindow;
struct TacEntity;
struct TacRenderer;
struct TacShell;
struct TacSoul;
struct TacTexture;
struct TacTextureAssetManager;
struct TacUI2DDrawData;
struct TacWindowParams;
struct TacWorld;


// this would be saved as a .map file in cod engine
struct TacPrefab
{
  TacVector< TacEntity* > mEntities;
  TacString mDocumentPath;
  //TacString GetDisplayName();
};

struct TacPrefabCameraPosition
{
  TacString mPrefab;
  TacCamera mCamera;
};


struct TacCreation
{
  static TacCreation* Instance;
  TacCreation();
  ~TacCreation();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  TacEntity* CreateEntity();
  bool IsAnythingSelected();
  v3 GetSelectionGizmoOrigin();
  void ClearSelection();

  // Prefabs
  void UpdateSavedPrefabs();
  void GetSavedPrefabs( TacVector< TacString > & paths, TacErrors& errors );
  void SavePrefabs();
  void LoadPrefabs( TacErrors& errors );
  void LoadPrefabAtPath( TacString path, TacErrors& errors );
  void LoadPrefabCameraPosition( TacPrefab* prefab );
  void SavePrefabCameraPosition( TacPrefab* prefab );
  void RemoveEntityFromPrefabRecursively( TacEntity* entity );
  TacPrefab* FindPrefab( TacEntity* entity );

  void ModifyPathRelative( TacString& path );
  void DeleteSelectedEntities();

  void CreatePropertyWindow( TacErrors& errors );
  void CreateGameWindow( TacErrors& errors );
  void CreateMainWindow( TacErrors& errors );
  void CreateSystemWindow( TacErrors& errors );
  void CreateProfileWindow( TacErrors& errors );

  void CreateDesktopWindow( TacString windowName, TacDesktopWindow** outDesktopWindow,  TacErrors& errors );
  bool ShouldCreateWindowNamed( TacJson* windows, const TacString& name );
  void GetWindowsJson( TacJson** outJson, TacErrors& errors );

  TacDesktopApp* mDesktopApp = nullptr;
  TacCreationMainWindow* mMainWindow = nullptr;
  TacCreationGameWindow* mGameWindow = nullptr;
  TacCreationPropertyWindow* mPropertyWindow = nullptr;
  TacCreationSystemWindow* mSystemWindow = nullptr;
  TacCreationProfileWindow* mProfileWindow = nullptr;

  TacString mOnlyCreateWindowNamed;

  TacWorld* mWorld = nullptr;

  // todo: TacHashSet
  TacVector< TacEntity* > mSelectedEntities;
  bool mSelectedHitOffsetExists = false;
  v3 mSelectedHitOffset = {};

  bool mSelectedGizmo = false;
  v3 mTranslationGizmoDir = {};
  float mTranslationGizmoOffset = 0;

  TacCamera mEditorCamera;

  // todo: TacHashSet
  TacVector< TacPrefab* > mPrefabs;
};

void SetCreationWindowImGuiGlobals(
  TacShell* shell,
  TacDesktopWindow* desktopWindow,
  TacUI2DDrawData* ui2DDrawData );

const v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );
