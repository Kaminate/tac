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

struct TacCreation
{
  ~TacCreation();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  TacEntity* CreateEntity();

  TacDesktopApp* mDesktopApp = nullptr;
  TacCreationMainWindow* mMainWindow = nullptr;
  TacCreationGameWindow* mGameWindow = nullptr;
  TacCreationPropertyWindow* mPropertyWindow = nullptr;

  TacWorld* mWorld = nullptr;
  TacEntity* mSelectedEntity = nullptr;
  bool mSelectedGizmo = false;
  v3 mTranslationGizmoDir = {};
  float mTranslationGizmoOffset = 0;

  TacCamera mEditorCamera;
};

const v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );
