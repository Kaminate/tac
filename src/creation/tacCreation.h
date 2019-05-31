#pragma once

#include "common/tacCamera.h"
#include "common/tacMemory.h"
#include "common/tacErrorHandling.h"
#include "common/tacSettings.h"
#include "common/tacEvent.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacRenderer.h"
#include "common/containers/tacVector.h"
#include "creation/tacCreationGameWindow.h"
#include "creation/tacCreationPropertyWindow.h"
#include "creation/tacCreationMainWindow.h"

struct TacCreation;
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



struct TacCreation
{
  ~TacCreation();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  TacEntity* CreateEntity();
  bool IsAnythingSelected();
  v3 GetSelectionGizmoOrigin();
  void ClearSelection();
  void UpdateSavedPrefabs();
  void GetSavedPrefabs( TacVector< TacString > & paths, TacErrors& errors );

  void SavePrefabs();
  TacJson SaveEntityToJsonRecusively( TacEntity* entity );

  void LoadPrefabs( TacErrors& errors );
  TacEntity* LoadEntityFromJsonRecursively( TacJson& prefabJson );

  void DeleteSelectedEntities();
  void DeleteEntity( TacEntity* entity );
  TacPrefab* FindPrefab( TacEntity* entity );

  TacDesktopApp* mDesktopApp = nullptr;
  TacCreationMainWindow* mMainWindow = nullptr;
  TacCreationGameWindow* mGameWindow = nullptr;
  TacCreationPropertyWindow* mPropertyWindow = nullptr;

  TacWorld* mWorld = nullptr;

  // todo: TacHashSet
  TacVector< TacEntity* > mSelectedEntities;

  bool mSelectedGizmo = false;
  v3 mTranslationGizmoDir = {};
  float mTranslationGizmoOffset = 0;

  TacCamera mEditorCamera;

  // todo: TacHashSet
  TacVector< TacPrefab* > mPrefabs;
};

const v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );
