#pragma once

#include "common/tacMemory.h"
#include "common/tacErrorHandling.h"
#include "common/tacRenderer.h"
#include "common/tacSettings.h"
#include "common/tacEvent.h"
#include "common/tacUI.h"
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
  void Init( TacErrors& errors );
  void Update( TacErrors& errors );
  void CreateEntity();

  TacDesktopApp* mDesktopApp = nullptr;
  TacCreationMainWindow* mMainWindow = nullptr;
  TacCreationGameWindow* mGameWindow = nullptr;
  TacCreationPropertyWindow* mPropertyWindow = nullptr;

  TacWorld* mWorld = nullptr;
  TacEntity* mSelectedEntity = nullptr;

  v3 mEditorCamPos;
  v3 mEditorCamForwards;
  v3 mEditorCamRight;
  v3 mEditorCamUp;
};

const v4 textColor = v4( v3( 1, 1, 1 ) * 0.0f, 1 );
