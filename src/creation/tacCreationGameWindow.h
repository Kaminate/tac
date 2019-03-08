#pragma once

#include "common/tacString.h"

struct TacErrors;
struct TacDesktopWindow;
struct TacCreation;
struct TacUIRoot;
struct TacUI2DDrawData;
struct TacSoul;
struct TacShell;
struct TacRenderView;

struct TacCreationGameWindow
{
  void Init( TacErrors& errors);
  void Update( TacErrors& errors );
  void RenderGameWorld();
  void SetImGuiGlobals();

  TacShell* mShell = nullptr;
  TacDesktopWindow* mDesktopWindow = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacSoul* mSoul = nullptr;
  TacCreation* mCreation = nullptr;
};


const TacString gGameWindowName = "VirtualGamePlayer";
